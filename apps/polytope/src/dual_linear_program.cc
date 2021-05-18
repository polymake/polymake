/* Copyright (c) 1997-2021
   Ewgenij Gawrilow, Michael Joswig, and the polymake team
   Technische Universität Berlin, Germany
   https://polymake.org

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version: http://www.gnu.org/licenses/gpl.txt.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
--------------------------------------------------------------------------------
*/

#include "polymake/client.h"
#include "polymake/Rational.h"
#include "polymake/Array.h"
#include "polymake/Set.h"
#include "polymake/Matrix.h"
#include "polymake/linalg.h"
#include "polymake/Integer.h"


namespace polymake { namespace polytope {

/*
 *  computes the dual linear program
 *  we get the problem as: min <c,x> s.t. Ax >= b, Bx = d
 *  we produce the dual: max <(b,d),(u,v)> s.t. A^t*u + B^t*v = c, u >= 0
 *  we have to take care of some signs if we maximize the primal problem.
 */
template <typename Scalar>
BigObject dual_linear_program(BigObject p_in, bool maximize)
{
   // variable declaration. Names are self explaining.
   Vector<Scalar> new_objective;
   Matrix<Scalar> old_ineq, new_ineq;
   Matrix<Scalar> old_eq, new_eq;
   Vector<Scalar> new_right;
   Int dim_dual_positive, dim_dual_free;
  
   // reading the old problem.
   // we get the problem as: min <c,x> s.t. Ax >= b, Bx = d
   BigObject old_lp = p_in.give("LP");
   old_lp.give("LINEAR_OBJECTIVE") >> new_right;
   p_in.lookup("FACETS | INEQUALITIES") >> old_ineq;
   p_in.lookup("AFFINE_HULL | EQUATIONS") >> old_eq;
  
   dim_dual_positive = old_ineq.rows();  // number of variables which must be >= 0
   dim_dual_free = old_eq.rows();        // number of variables which do not have a sign constrain

   // building the new Problem. Starting with the new objective = (b,d)
   new_objective = zero_vector<Scalar>(1);
   if (dim_dual_positive > 0) {
      new_objective |= (-old_ineq.col(0));
   }
   if (dim_dual_free > 0) {
      new_objective |= (-old_eq.col(0));
   }
  
   // taking care of correct sign for objective and right hand side
   // if we want to maximize the primal problem
   if (maximize) {
      new_objective.negate();
      new_right.negate();
   }

   // building the new equations = (A^t, B^t) (u,v) = c
   new_eq |= -new_right.slice(range_from(1));
   if (dim_dual_positive > 0)
      new_eq |= T(old_ineq.minor(All, range_from(1)));
   if (dim_dual_free > 0)
      new_eq |= T(old_eq.minor(All, range_from(1)));

   // building the new inequalities which are just the sign constrains for "u"
   // and the inequality for the far-face
   new_ineq = (unit_matrix<Scalar>(dim_dual_positive+1));
   if (dim_dual_free > 0) {
      new_ineq |= (zero_matrix<Scalar>(dim_dual_positive+1, dim_dual_free));
   }

   // writing the new problem  
   return BigObject("Polytope", mlist<Scalar>(),
                    "INEQUALITIES", new_ineq,
                    "EQUATIONS", new_eq,
                    "LP.LINEAR_OBJECTIVE", new_objective);
}

UserFunctionTemplate4perl("# @category Producing a polytope from polytopes"
                          "# Produces the dual linear program for a given linear program of the form min {cx | Ax >= b, Bx = d}."
                          "# Here (A,b) are given by the FACETS (or the INEQAULITIES), and (B,d) are given by the AFFINE_HULL"
                          "# (or by the EQUATIONS) of the polytope P, while the objective function c comes from an LP subobject."
                          "# @param Polytope P = {x | Ax >= b, Bx = d}"
                          "# @param Bool maximize tells if the primal lp is a maximization problem. Default value is 0 (= minimize)"
                          "# @return Polytope", 
                          "dual_linear_program<Scalar> [ is_ordered_field(type_upgrade<Scalar, Rational>) ](Polytope<type_upgrade<Scalar>>; $=0)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
