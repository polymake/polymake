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
#include "polymake/polytope/to_interface.h"


namespace polymake { namespace polytope {

template <typename Scalar>
bool to_input_feasible (perl::Object p) {
   Matrix<Scalar> I = p.lookup("FACETS | INEQUALITIES"),
                  E = p.lookup("LINEAR_SPAN | EQUATIONS");

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


      // BOUNDED is decided by determining whether [1,0,0,0,...] is a strictly interior point of the cone C spanned by INEQUALITIES and +/-EQUATIONS
      // to cope for the case that C is low dimensional we ask for lineality space first
      // the primal linear program is then to determine whether 
      // max lambda
      // y^t (F/E/-E)=e_0
      // y_i >= lambda
      // has a positive maximal value
template <typename Scalar>
bool to_input_bounded  (perl::Object p) {
   const Matrix<Scalar> L = p.give("LINEALITY_SPACE");
   if ( L.rows() > 0 ) return false;

   Matrix<Scalar> F = p.give("FACETS | INEQUALITIES"),
                  E = p.lookup("AFFINE_HULL | EQUATIONS");

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




template <typename Scalar>
void to_solve_lp(perl::Object p, perl::Object lp, bool maximize)
{
   typedef to_interface::solver<Scalar> Solver;
   const Matrix<Scalar> H=p.give("FACETS | INEQUALITIES"),
      E=p.lookup("AFFINE_HULL | EQUATIONS");
   const Vector<Scalar> Obj=lp.give("LINEAR_OBJECTIVE");

   try {
      Solver solver;
      typename Solver::lp_solution S=solver.solve_lp(H, E, Obj, maximize);
      lp.take(maximize ? "MAXIMAL_VALUE" : "MINIMAL_VALUE") << S.first;
      lp.take(maximize ? "MAXIMAL_VERTEX" : "MINIMAL_VERTEX") << S.second;
      p.take("FEASIBLE") << true;
   }
   catch (infeasible) {
      lp.take(maximize ? "MAXIMAL_VALUE" : "MINIMAL_VALUE") << perl::undefined();
      lp.take(maximize ? "MAXIMAL_VERTEX" : "MINIMAL_VERTEX") << perl::undefined();
      p.take("FEASIBLE") << false;
   }
   catch (unbounded) {
      if (maximize)
         lp.take("MAXIMAL_VALUE") << std::numeric_limits<Scalar>::infinity();
      else
         lp.take("MINIMAL_VALUE") << -std::numeric_limits<Scalar>::infinity();
      lp.take(maximize ? "MAXIMAL_VERTEX" : "MINIMAL_VERTEX") << perl::undefined();
      p.take("FEASIBLE") << true;
   }
}


FunctionTemplate4perl("to_input_feasible<Scalar> (Polytope<Scalar>)");
FunctionTemplate4perl("to_input_bounded<Scalar> (Polytope<Scalar>)");
FunctionTemplate4perl("to_solve_lp<Scalar> (Polytope<Scalar>, LinearProgram<Scalar>, $) : void");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
