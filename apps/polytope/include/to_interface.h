/* Copyright (c) 1997-2015
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

#ifndef POLYMAKE_POLYTOPE_TO_INTERFACE_H
#define POLYMAKE_POLYTOPE_TO_INTERFACE_H

#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/polytope/lpch_dispatcher.h"
#include "polymake/Set.h"

namespace polymake { namespace polytope { namespace to_interface {

template <typename Coord>
class solver {
public:
   typedef Coord coord_type;

   solver();

   Set<int> initial_basis;

   typedef std::pair<coord_type, Vector<coord_type> > lp_solution;

   /// @retval first: objective value, second: solution
   // LP only defined for polytopes
   // CAUTION: to interpret the return value notice that this is a *dual* simplex
   lp_solution
   solve_lp(const Matrix<coord_type>& Inequalities, const Matrix<coord_type>& Equations,
            const Vector<coord_type>& Objective, bool maximize);

   void set_basis(const Set<int>& basis);

#if POLYMAKE_DEBUG
   bool debug_print;
#endif
};

template <typename Scalar>
bool to_input_feasible_impl (const Matrix<Scalar>& I,
                             const Matrix<Scalar>& E) 
{
   const int d = std::max(I.cols(),E.cols());
   if (d == 0)
      return true;

   typedef to_interface::solver<Scalar> Solver;
   try {
      Vector<Scalar> obj = unit_vector<Scalar>(I.cols(),0);
      Solver solver;
      typename Solver::lp_solution S=solver.solve_lp(I, E, obj, true);
   } 
   catch (infeasible) {
      return false;
   }
   catch (unbounded) {
      return true;
   } 
   return true;
}

template <typename Scalar>
bool to_input_bounded_impl(const Matrix<Scalar>& L,
                           const Matrix<Scalar>& F,
                           const Matrix<Scalar>& E) 
{
   int r = F.cols();
   Matrix<Scalar> Eq;
   if ( E.rows() ) { // FIXME write more efficiently
      Eq = vector2col(-(unit_vector<Scalar>(r,0)))|vector2col(zero_vector<Scalar>(r))|T(F/E/-E);
   } else {
      Eq = vector2col(-(unit_vector<Scalar>(r,0)))|vector2col(zero_vector<Scalar>(r))|T(F);
   }
   r = Eq.cols()-2;
   Matrix<Scalar> Ineq = vector2col(zero_vector<Scalar>(r))|vector2col(-(ones_vector<Scalar>(r)))|((unit_matrix<Scalar>(r)));
   Vector<Scalar> v = (unit_vector<Scalar>(r+2,1));

   typedef to_interface::solver<Scalar> Solver;
   try {
      Solver solver;
      typename Solver::lp_solution S=solver.solve_lp(Ineq, Eq, v, true);
      return S.first > 0 ? true : false;
   } 
   catch ( infeasible ) {
      return true;     // the dual solution is unbounded, so the original problem is infeasible. 
   }
   return true;
}

} } }

#endif // POLYMAKE_POLYTOPE_TO_INTERFACE_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
