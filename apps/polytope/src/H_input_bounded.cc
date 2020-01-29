/* Copyright (c) 1997-2020
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
#include "polymake/polytope/solve_LP.h"

namespace polymake { namespace polytope {

/*
   BOUNDED is decided by determining whether [1,0,0,0,...] is a strictly interior point of the cone C spanned by INEQUALITIES and +/-EQUATIONS
   to cope with the case where C is low dimensional, we ask for lineality space first.
   The primal linear program is then to determine whether
     max lambda
     y^t (F/E/-E)=e_0
     y_i >= lambda
   has a positive maximal value.
   To reduce dimension we solve the dual lp instead. 
   Note that the dual LP is slightly transformed:
   the inequalities in its standard form are (with M=F/E/-E)
     x^tM^t=z^t
     z^t\1=1
     z\ge 0
   which we write as
     \1^tMx=1
     Fx\ge 0
*/
template <typename Scalar>
bool H_input_bounded(BigObject p)
{
   const Matrix<Scalar> L = p.give("LINEALITY_SPACE");
   if (L.rows() > 0) return false;

   Matrix<Scalar> F = p.give("FACETS | INEQUALITIES"),
                  E = p.lookup("AFFINE_HULL | EQUATIONS");

   if (F.cols() != E.cols() &&
       F.cols() && E.cols())
      throw std::runtime_error("H_input_bounded - dimension mismatch between Inequalities and Equations");

   F = zero_vector<Scalar>()|F;
   if (E.cols())
      E = zero_vector<Scalar>()|E;

   Vector<Scalar> v = (ones_vector<Scalar>(F.rows()))*F;
   v[0] = -1;
   E /= v;
   const Vector<Scalar> obj = unit_vector<Scalar>(F.cols(), 1);
   const LP_Solution<Scalar> S = solve_LP(F, E, obj, false);
   return S.status == LP_status::valid && S.objective_value > 0 || S.status == LP_status::infeasible;
}

FunctionTemplate4perl("H_input_bounded<Scalar> (Polytope<Scalar>)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
