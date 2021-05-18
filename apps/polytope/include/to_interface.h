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

#pragma once

#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/Set.h"
#include "polymake/polytope/solve_LP.h"
#if POLYMAKE_DEBUG
#  include "polymake/vector"
#  include "polymake/client.h"
#endif

#define TO_DISABLE_OUTPUT
#include "polymake/common/TOmath_decl.h"
#include "TOSimplex/TOSimplex.h"

namespace polymake { namespace polytope { namespace to_interface {

template <typename Coord>
class Solver : public LP_Solver<Coord> {
public:
   Solver()
#if POLYMAKE_DEBUG
      : debug_print(get_debug_level() > 1)
#endif
   {}

   // CAUTION: to interpret the return value notice that this is a *dual* simplex
   LP_Solution<Coord>
   solve(const Matrix<Coord>& Inequalities, const Matrix<Coord>& Equations,
         const Vector<Coord>& Objective, bool maximize, const Set<Int>& initial_basis) const;

   LP_Solution<Coord>
   solve(const Matrix<Coord>& Inequalities, const Matrix<Coord>& Equations,
         const Vector<Coord>& Objective, bool maximize, bool) const override
   {
      return Solver::solve(Inequalities, Equations, Objective, maximize, Set<Int>());
   }

#if POLYMAKE_DEBUG
private:
   const bool debug_print;
#endif
};

namespace {

template <typename Coord>
void to_dual_fill_data(const Matrix<Coord>& A1, const Matrix<Coord>& A2, const std::vector<Coord>& MinObjective,
                       std::vector<Coord>& to_coefficients, std::vector<Int>& to_colinds, std::vector<Int>& to_rowbegininds,
                       std::vector<TOSimplex::TORationalInf<Coord> >& to_rowlowerbounds, std::vector<TOSimplex::TORationalInf<Coord> >& to_rowupperbounds,
                       std::vector<TOSimplex::TORationalInf<Coord> >& to_varlowerbounds, std::vector<TOSimplex::TORationalInf<Coord> >& to_varupperbounds,
                       std::vector<Coord>& to_objective)
{
   Int nnz = 0;
   for (Int i = 0; i < A1.rows(); ++i) {
      for (Int j = 1; j < A1.cols(); ++j) {
         if (!is_zero(A1(i,j))){
            ++nnz;
         }
      }
   }
   for (Int i = 0; i < A2.rows(); ++i) {
      for (Int j = 1; j < A2.cols(); ++j) {
         if (!is_zero(A2(i,j))){
            ++nnz;
         }
      }
   }
   to_coefficients.reserve(nnz);
   to_colinds.reserve(nnz);

   /*
     note: polymake input with rows [c_i, a_i] and [c_j, a_j] with a_i=-a_j
     could be reduced to one column with 'to_rowupperbounds' and 'to_rowlowerbounds'.
   */

   const Int m = A1.cols()-1;
   const Int n1 = A1.rows();
   const Int n2 = A2.rows();
   const Int n = n1+n2;

   assert(A2.cols() == 0 || m == A2.cols()-1);

   to_rowbegininds.reserve(m+1);

   for (Int i = 1; i <= m; ++i) {
      to_rowbegininds.push_back(to_coefficients.size());
      for (Int j = 0; j < n1; ++j) {
         if (!is_zero(A1(j, i))) {
            to_coefficients.push_back(A1(j, i));
            to_colinds.push_back(j);
         }
      }
      for (Int j = 0; j < n2; ++j) {
         if (!is_zero(A2(j, i))) {
            to_coefficients.push_back(A2(j, i));
            to_colinds.push_back(n1+j);
         }
      }
   }
   to_rowbegininds.push_back(to_coefficients.size());

   to_rowupperbounds.reserve(m);
   for (Int i = 0; i < m; ++i) {
      to_rowupperbounds.push_back(MinObjective[i]); // A'u + E'v =< c
   }
   to_rowlowerbounds = to_rowupperbounds; // A'u + E'v >= c

   to_objective.reserve(n);
   to_varlowerbounds.reserve(n);
   to_varupperbounds.reserve(n);
   for (Int i = 0; i < n1; ++i) {
      to_objective.push_back(A1(i, 0));       // = b_i
      to_varlowerbounds.push_back(Coord(0)); // u_i >= 0
      to_varupperbounds.push_back(true);     // true = no restriction
   }
   for (Int i = 0; i < n2; ++i) {
      to_objective.push_back(A2(i,0));  // = d_i
      to_varlowerbounds.push_back(true);
      to_varupperbounds.push_back(true);
   }
}

} // end anonymous namespace



/* This method uses the TOSimplex-LP-solver to solve the LP.
 * TOSimplex is a dual LP solver, so it cannot decide whether a given LP is infeasible or unbounded.
 * To avoid this problem, the LP is implicitly dualized:
 *
 * min c'x             - min -b'u -d'v
 * s.t. Ax >= b  <==>  s.t. A'u + E'v = c
 *      Ex == d             u >= 0
 *
 * After solving, the dual variables are obtained, if the problem is feasible.
 * note: restrictions on variables (in this case u >= 0) are handeld
 * via 'to_varupperbounds' and 'to_varlowerbounds'.
 */

template <typename Coord>
LP_Solution<Coord>
Solver<Coord>::solve(const Matrix<Coord>& Inequalities, const Matrix<Coord>& Equations,
                     const Vector<Coord>& Objective, bool maximize, const Set<Int>& initial_basis) const
{
   using to_solver = TOSimplex::TOSolver<Coord,Int>;

   const Int n = Inequalities.cols()-1; // beware of leading constant term

#if POLYMAKE_DEBUG
   if (debug_print) {
      const Int m = Inequalities.rows() + Equations.rows();
      cout << "to_solve_lp(m=" << m << " n=" << n << ")" << endl;
   }
#endif

   // translate inequality constraints into sparse description
   std::vector<Coord> to_coefficients;
   std::vector<Int> to_colinds, to_rowbegininds;
   std::vector<TOSimplex::TORationalInf<Coord>> to_rowlowerbounds, to_rowupperbounds, to_varlowerbounds, to_varupperbounds;
   std::vector<Coord> to_objective;

   // translate objective function into dense description, take direction into account
   std::vector<Coord> to_obj(n);
   if (maximize) {
      for (Int j = 1; j <= n; ++j)
         to_obj[j-1] = -Objective[j];
   } else {
      for (Int j = 1; j <= n; ++j)
         to_obj[j-1] = Objective[j];
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

   to_solver solver(to_coefficients, to_colinds, to_rowbegininds, to_objective, to_rowlowerbounds, to_rowupperbounds, to_varlowerbounds, to_varupperbounds);


   // add start basis:
   if (!initial_basis.empty()) {
      const Int m = Inequalities.rows() + Equations.rows();
      std::vector<Int> b1(m);
      std::vector<Int> b2(n); //slack variables
      /*
        interpretation of the entries
        0 : the value is between the bounds (non strict)
        1 : the lower bound is attained
        2 : the upper bound is attained
       */

      Int count = Inequalities.cols()-1;
      for (const Int i : initial_basis) {
         b1[i] = 1;
         if (--count <= 0) break;
      }
      solver.setBase(b1, b2);
   }

   LP_Solution<Coord> result;
   switch (solver.opt()) {
   case 0: {
      // solved to optimality
      result.status = LP_status::valid;
      result.objective_value = solver.getObj();
      if (!maximize) negate(result.objective_value);
      const std::vector<Coord>& tox(solver.getY());
      result.solution.resize(1+n);
      result.solution[0] = one_value<Coord>();
      for (Int j = 1; j <= n; ++j)
         result.solution[j] = -tox[j-1]; // It seems that the duals have an unexpected sign, so let's negate them
      break;
   }
   case 1:
      result.status = LP_status::unbounded;
      break;
   default:
      result.status = LP_status::infeasible;
      break;
   }

   return result;
}

} } }


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
