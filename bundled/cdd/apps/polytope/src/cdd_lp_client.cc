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

#include "polymake/client.h"
#include "polymake/polytope/cdd_interface.h"

namespace polymake { namespace polytope {

template <typename Scalar>
void cdd_solve_lp(perl::Object p, perl::Object lp, bool maximize)
{
   typedef cdd_interface::solver<Scalar> Solver;
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
   catch (unbounded) {
      if (maximize)
         lp.take("MAXIMAL_VALUE") << std::numeric_limits<Scalar>::infinity();
      else
         lp.take("MINIMAL_VALUE") << -std::numeric_limits<Scalar>::infinity();
      lp.take(maximize ? "MAXIMAL_VERTEX" : "MINIMAL_VERTEX") << perl::undefined();
      p.take("FEASIBLE") << true;
   }
   catch (infeasible) {
      lp.take(maximize ? "MAXIMAL_VALUE" : "MINIMAL_VALUE") << perl::undefined();
      lp.take(maximize ? "MAXIMAL_VERTEX" : "MINIMAL_VERTEX") << perl::undefined();
      p.take("FEASIBLE") << false;
   }
}


template <typename Scalar>
bool cdd_input_feasible (perl::Object p) {
   Matrix<Scalar> I = p.lookup("FACETS | INEQUALITIES"),
                  E = p.lookup("LINEAR_SPAN | EQUATIONS");

   const int d = std::max(I.cols(),E.cols());
   if (d == 0)
      return true;

   typedef cdd_interface::solver<Scalar> Solver;
   try {
      Vector<Scalar> obj = unit_vector<Scalar>(d,0);
      Solver solver;
      typename Solver::lp_solution S=solver.solve_lp(I, E, obj, true);
   } 
   catch ( infeasible ) {
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
      // to reduce dimension we solve the dual lp instead. 
      // note that the dual LP is slightly transformed
      // the inequalities in its standard form are (with M=F/E/-E)
      // x^tM^t=z^t
      // z^t\1=1
      // z\ge 0
      // which we write as
      //\1^tMx=1
      //Fx\ge 0
template <typename Scalar>
bool cdd_input_bounded  (perl::Object p) {
   const Matrix<Scalar> L = p.give("LINEALITY_SPACE");
   if ( L.rows() > 0 ) return false;

   Matrix<Scalar> F = p.give("FACETS | INEQUALITIES"),
                  E = p.lookup("AFFINE_HULL | EQUATIONS");

   F = vector2col(zero_vector<Scalar>(F.rows()))|F;
   E = vector2col(zero_vector<Scalar>(E.rows()))|E;
   Vector<Scalar> v = (ones_vector<Scalar>(F.rows()))*F;
   v[0]=-1;
   E /= v;

   typedef cdd_interface::solver<Scalar> Solver;
   try {
      Vector<Scalar> obj = unit_vector<Scalar>(F.cols(),1);
      Solver solver;
      typename Solver::lp_solution S=solver.solve_lp(F, E, obj, false);
      return S.first > 0 ? true : false;
   } 
   catch ( infeasible ) {
      return true;
   }
   catch (unbounded) {
      return true;
   } 
   return true;
}

      // try to compute a separating hyperplane
template <typename Scalar>
bool polytope_contains_point  (perl::Object p, const Vector<Scalar> & v) {
   
   Matrix<Scalar> F = p.give("VERTICES | POINTS");
   Matrix<Scalar> E(0,F.cols());
   p.lookup("LINEALITY_SPACE | INPUT_LINEALITY") >> E;

   // the lp solver does not check dimensions
   if (F.cols() != v.dim() )
      throw std::runtime_error("polytope - point dimension mismatch");
   
   F = vector2col(zero_vector<Scalar>(F.rows()))|F;
   if ( E.rows() > 0 ) { E = vector2col(zero_vector<Scalar>(E.rows()))|E; } else { E.resize(0,F.cols()); }
   F /= (1|v);
   Vector<Scalar> c = 0|v;
   
   typedef cdd_interface::solver<Scalar> Solver;
   try {
      Solver solver;
      typename Solver::lp_solution S=solver.solve_lp(F, E, c, false);
      return ( S.first < 0 ) ? false : true;
   } 
   catch ( infeasible ) {
      return true;
   }
   catch (unbounded) {
      return false;
   } 
   return true;
}


FunctionTemplate4perl("polytope_contains_point<Scalar> (Polytope<Scalar>, Vector<Scalar>)");
FunctionTemplate4perl("cdd_input_bounded<Scalar> (Polytope<Scalar>)");
FunctionTemplate4perl("cdd_input_feasible<Scalar> (Polytope<Scalar>)");
FunctionTemplate4perl("cdd_solve_lp<Scalar> (Polytope<Scalar>, LinearProgram<Scalar>, $) : void");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
