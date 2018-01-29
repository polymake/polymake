/* Copyright (c) 1997-2018
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
bool cdd_input_feasible(perl::Object p);

template <typename Scalar>
void cdd_solve_lp(perl::Object p, perl::Object lp, bool maximize)
{
   typedef cdd_interface::solver<Scalar> Solver;
   std::string H_name;
   const Matrix<Scalar> H=p.give_with_property_name("FACETS | INEQUALITIES", H_name),
                        E=p.lookup("AFFINE_HULL | EQUATIONS");
   const Vector<Scalar> Obj=lp.give("LINEAR_OBJECTIVE");

   if (H.cols() != E.cols() &&
       H.cols() && E.cols())
      throw std::runtime_error("cdd_solve_lp - dimension mismatch between Inequalities and Equations");

   bool is_unbounded = 0, is_infeasible = 0;
   try {
      Solver solver;
      typename Solver::lp_solution S=solver.solve_lp(H, E, Obj, maximize);
      lp.take(maximize ? "MAXIMAL_VALUE" : "MINIMAL_VALUE") << S.first;
      lp.take(maximize ? "MAXIMAL_VERTEX" : "MINIMAL_VERTEX") << S.second;
      p.take("FEASIBLE") << true;
   }
   catch (unbounded) {
      is_unbounded = 1;
   }
   catch (infeasible) {
      is_infeasible = 1;
   }
   catch (baddual) {
      if (H_name.compare("FACETS") == 0){
         // if we've got facets we check wether they are empty.
         // empty facets => infeasible polytope
         // there are some facets => it is feasible, thus it must be unbounded
         if(H.rows()>0){
            is_unbounded = 1;
         }else{
            is_infeasible = 1;
         }
      }else{
         // if we've got inequalities: we have to do a feasible check.
         if(cdd_input_feasible<Scalar>(p)){
            is_unbounded = 1;
         }else{
            is_infeasible = 1;
         }
      }
   }

   if(is_unbounded){
      if (maximize)
         lp.take("MAXIMAL_VALUE") << std::numeric_limits<Scalar>::infinity();
      else
         lp.take("MINIMAL_VALUE") << -std::numeric_limits<Scalar>::infinity();
      lp.take(maximize ? "MAXIMAL_VERTEX" : "MINIMAL_VERTEX") << perl::undefined();
      p.take("FEASIBLE") << true;
   }else if(is_infeasible){
      lp.take(maximize ? "MAXIMAL_VALUE" : "MINIMAL_VALUE") << perl::undefined();
      lp.take(maximize ? "MAXIMAL_VERTEX" : "MINIMAL_VERTEX") << perl::undefined();
      p.take("FEASIBLE") << false;
   }
}


template <typename Scalar>
bool cdd_input_feasible(perl::Object p)
{
   Matrix<Scalar> I = p.lookup("FACETS | INEQUALITIES"),
                  E = p.lookup("LINEAR_SPAN | EQUATIONS");

   if (I.cols() != E.cols() &&
       I.cols() && E.cols())
      throw std::runtime_error("cdd_input_feasible - dimension mismatch between Inequalities and Equations");

   const int d = std::max(I.cols(), E.cols());
   if (d == 0)
      return true;

   typedef cdd_interface::solver<Scalar> Solver;
   try {
      Vector<Scalar> obj = unit_vector<Scalar>(d,0);
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
bool cdd_input_bounded(perl::Object p)
{
   const Matrix<Scalar> L = p.give("LINEALITY_SPACE");
   if ( L.rows() > 0 ) return false;

   Matrix<Scalar> F = p.give("FACETS | INEQUALITIES"),
                  E = p.lookup("AFFINE_HULL | EQUATIONS");

   if (F.cols() != E.cols() &&
       F.cols() && E.cols())
      throw std::runtime_error("cdd_input_bounded - dimension mismatch between Inequalities and Equations");

   F = zero_vector<Scalar>()|F;
   if (E.cols())
      E = zero_vector<Scalar>()|E;

   Vector<Scalar> v = (ones_vector<Scalar>(F.rows()))*F;
   v[0]=-1;
   E /= v;

   typedef cdd_interface::solver<Scalar> Solver;
   try {
      Vector<Scalar> obj = unit_vector<Scalar>(F.cols(),1);
      Solver solver;
      typename Solver::lp_solution S=solver.solve_lp(F, E, obj, false);
      return S.first > 0;
   } 
   catch (infeasible) {
      return true;
   }
   catch (unbounded) {
      return true;
   } 
   return true;
}

FunctionTemplate4perl("cdd_input_bounded<Scalar> (Polytope<Scalar>)");
FunctionTemplate4perl("cdd_input_feasible<Scalar> (Polytope<Scalar>)");
FunctionTemplate4perl("cdd_solve_lp<Scalar> (Polytope<Scalar>, LinearProgram<Scalar>, $) : void");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
