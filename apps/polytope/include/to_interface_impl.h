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

#ifndef POLYMAKE_POLYTOPE_TO_INTERFACE_IMPL_H
#define POLYMAKE_POLYTOPE_TO_INTERFACE_IMPL_H

#include "polymake/polytope/to_interface.h"
#if POLYMAKE_DEBUG
#  include "polymake/client.h"
#  include "polymake/vector"
#endif

#define TO_WITHOUT_DOUBLE
#define TO_DISABLE_OUTPUT
// the following requires an entry in Makefile.inc
#include "TOSimplex.h"

namespace polymake {

namespace {

   template <typename Coord>
   void to_dual_fill_data(const Matrix<Coord>& A1, const Matrix<Coord>& A2, const std::vector<Coord>& MinObjective,
                      std::vector<Coord>& to_coefficients, std::vector<int>& to_colinds, std::vector<int>& to_rowbegininds,
                      std::vector<TOSimplex::TORationalInf<Coord> >& to_rowlowerbounds, std::vector<TOSimplex::TORationalInf<Coord> >& to_rowupperbounds,
                      std::vector<TOSimplex::TORationalInf<Coord> >& to_varlowerbounds, std::vector<TOSimplex::TORationalInf<Coord> >& to_varupperbounds,
                      std::vector<Coord>& to_objective) {
      
      unsigned int nnz = 0;
      for (int i=0; i<A1.rows(); ++i) {
         for (int j=1; j<A1.cols(); ++j) {
            if (!is_zero(A1(i,j))){
               ++nnz;
            }
         }
      }
      for (int i=0; i<A2.rows(); ++i) {
         for (int j=1; j<A2.cols(); ++j) {
            if (!is_zero(A1(i,j))){
               ++nnz;
            }
         }
      }
      to_coefficients.reserve(nnz);
      to_colinds.reserve(nnz);
      
      
      const unsigned int m = A1.cols() - 1;
      const unsigned int n1 = A1.rows();
      const unsigned int n2 = A2.rows();
      const unsigned int n = n1 + n2;
      assert( (A2.cols() == 0) || (m == (unsigned int)( A2.cols() - 1 )) );
      
      to_rowbegininds.reserve(m+1);
      
      for (unsigned int i=1; i<=m; ++i){
         to_rowbegininds.push_back(to_coefficients.size());
         for (unsigned int j=0; j<n1; ++j){
            if (!is_zero(A1(j,i))){
               to_coefficients.push_back(A1(j,i));
               to_colinds.push_back(j);
            }
         }
         for (unsigned int j=0; j<n2; ++j){
            if (!is_zero(A2(j,i))){
               to_coefficients.push_back(A2(j,i));
               to_colinds.push_back(n1+j);
            }
         }
      }
      to_rowbegininds.push_back(to_coefficients.size());
      
      to_rowupperbounds.reserve(m);
      for (unsigned int i=0; i<m; ++i){
         to_rowupperbounds.push_back(MinObjective[i]);
      }
      to_rowlowerbounds = to_rowupperbounds;
      
      to_objective.reserve(n);
      to_varlowerbounds.reserve(n);
      to_varupperbounds.reserve(n);
      for (unsigned int i=0; i<n1; ++i){
         to_objective.push_back(A1(i,0));
         to_varlowerbounds.push_back(Coord(0));
         to_varupperbounds.push_back(true);
      }
      for (unsigned int i=0; i<n2; ++i){
         to_objective.push_back(A2(i,0));
         to_varlowerbounds.push_back(true);
         to_varupperbounds.push_back(true);
      }
   }
} // end anonymous namespace


namespace polytope { namespace to_interface {

template <typename Coord>
solver<Coord>::solver()
{
#if POLYMAKE_DEBUG
   debug_print = perl::get_debug_level() > 1;
#endif
}


/* This method uses the TOSimplex-LP-solver to solve the LP.
 * TOSimplex is a dual LP solver, so it cannot decide whether a given LP is infeasible or unbounded.
 * To avoid this problem, the LP is implicitly dualized:
 *
 * min c'x             - min -b'u -d'v
 * s.t. Ax >= b  <==>  s.t. A'u + E'v = c
 *      Ex == d             u >= 0
 *
 * After solving, the dual variables are obtained, if the problem is feasible.
 */

template <typename Coord>
typename solver<Coord>::lp_solution
solver<Coord>::solve_lp(const Matrix<Coord>& Inequalities, const Matrix<Coord>& Equations,
                        const Vector<Coord>& Objective, bool maximize)
{
   typedef TOSimplex::TOSolver<Coord> to_solver;

   const int n(Inequalities.cols()-1); // beware of leading constant term

#if POLYMAKE_DEBUG
   if (debug_print) {
      const int m(Inequalities.rows()+Equations.rows());
      cout << "to_solve_lp(m=" << m << " n=" << n << ")" << endl;
   }
#endif

   // translate inequality constraints into sparse description
   std::vector<Coord> to_coefficients;
   std::vector<int> to_colinds, to_rowbegininds;
   std::vector<TOSimplex::TORationalInf<Coord> > to_rowlowerbounds, to_rowupperbounds, to_varlowerbounds, to_varupperbounds;
   std::vector<Coord> to_objective;

   // translate objective function into dense description, take direction into account
   std::vector<Coord> to_obj(n);
   if (maximize) {
      for (int j=1; j<=n; ++j) to_obj[j-1]=-Objective[j];
   } else {
      for (int j=1; j<=n; ++j) to_obj[j-1]=Objective[j];
   }

   // Fill the vectors with the dual problem
   to_dual_fill_data(Inequalities, Equations, to_obj, to_coefficients, to_colinds, to_rowbegininds, to_rowlowerbounds, to_rowupperbounds, to_varlowerbounds, to_varupperbounds, to_objective );
   
#if POLYMAKE_DEBUG
   if (debug_print)
      cout << "to_coefficients:" << to_coefficients <<"\n"
              "to_colinds:" << to_colinds << "\n"
              "to_rowbegininds:" << to_rowbegininds << "\n"
              "to_obj:" << to_obj << "\n"
              "to_objective:" << to_objective << endl;
#endif

   to_solver solver( to_coefficients, to_colinds, to_rowbegininds, to_objective, to_rowlowerbounds, to_rowupperbounds, to_varlowerbounds, to_varupperbounds );
   const int result (solver.opt());

   if (result==0) { // solved to optimality
      Coord val(solver.getObj());
      if (!maximize) val=-val;
      const std::vector<Coord>& tox(solver.getY());
      Vector<Coord> x(1+n);
      x[0]=Coord(1); for (int j=1; j<=n; ++j) x[j]=-tox[j-1]; // It seems that the duals have an unexpected sign, so let's negate them
      return lp_solution(val,x);
   } else if (result==1) {
      throw unbounded();
   } else {
      throw infeasible();
   }
}

} } }

#endif // POLYMAKE_POLYTOPE_TO_INTERFACE_IMPL_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
