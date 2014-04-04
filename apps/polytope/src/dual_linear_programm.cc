/* Copyright (c) 1997-2014
   Ewgenij Gawrilow, Michael Joswig (Technische Universitaet Berlin, Germany)
   http://www.polymake.org

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
perl::Object dual_linear_program(perl::Object p_in, bool maximize)
{
  // variable declaration. Names are self explaining.
  perl::Object old_lp, new_lp(perl::ObjectType::construct<Rational>("LinearProgram"));
  perl::Object p_out(perl::ObjectType::construct<Rational>("Polytope"));
  Vector<Rational> new_objective;
  Matrix<Rational> old_ineq, new_ineq;
  Matrix<Rational> old_eq, new_eq;
  Vector<Rational> new_right;
  int dim_dual_positive, dim_dual_free;
  
  // reading the old problem.
  // we get the problem as: min <c,x> s.t. Ax >= b, Bx = d
  p_in.give("LP") >> old_lp;
  old_lp.give("LINEAR_OBJECTIVE") >> new_right;
  p_in.lookup("FACETS | INEQUALITIES") >> old_ineq;
  p_in.lookup("EQUATIONS | AFFINE_HULL") >> old_eq;
  
  dim_dual_positive = old_ineq.rows();  // number of variables which must be >= 0
  dim_dual_free = old_eq.rows();        // number of variables which do not have a sign constrain
  
  
  // building the new Problem. Starting with the new objective = (b,d)
  new_objective = zero_vector<Rational>(1);
  if (dim_dual_positive > 0) {new_objective |= (-old_ineq.col(0));};
  if (dim_dual_free > 0) {new_objective |= (-old_eq.col(0));};
  
  // taking care of correct sign for objective and right hand side
  // if we want to maximize the primal problem
  if (maximize) {
    new_objective = -new_objective;
    new_right = -new_right;
  }
  
  // building the new equations = (A^t, B^t) (u,v) = c
  new_eq |= (-new_right.slice(~scalar2set(0)));
  if (dim_dual_positive > 0) {new_eq |= (T(old_ineq.minor(All,~scalar2set(0))));};
  if (dim_dual_free > 0) {new_eq |= (T(old_eq.minor(All,~scalar2set(0))));};
  
  // building the new inequalities which are just the sign constrains for "u"
  // and the inequality for the far-face
  new_ineq = (unit_matrix<Rational>(dim_dual_positive + 1));
  if (dim_dual_free >0) {new_ineq |= (zero_matrix<Rational>(dim_dual_positive+1, dim_dual_free));};

  // writing the new problem  
  new_lp.take("LINEAR_OBJECTIVE") << new_objective;
  p_out.take("INEQUALITIES") << new_ineq;
  p_out.take("EQUATIONS") << new_eq;
  p_out.take("LP") << new_lp;

  return p_out;
}


UserFunction4perl("# @category Producing a polytope from polytopes"
                  "# Produces a polyhedron with an totally dual integral inequality formulation of a full dimensional polyhedron"
                  "# @param Polytope P"
                  "# @param bool maximize weather we maximize our primal problem or not. Default value is 0 (= minimize)"
                  "# @return Polytope", 
                  &dual_linear_program, "dual_linear_program(Polytope; $=0)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
