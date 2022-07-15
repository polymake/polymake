/* Copyright (c) 1997-2022
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

#include "polymake/polytope/soplex_interface.h"
#include "polymake/Rational.h"

// we keep this for the whole file because of various =0 default arguments...
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#endif

// this is only for the include below
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wconversion"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#if __GNUC__ >= 12
#pragma GCC diagnostic ignored "-Wuse-after-free"
#endif
#endif

#define SOPLEX_WITH_GMP
#include "soplex.h"

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

namespace polymake { namespace polytope { namespace soplex_interface {

/** This method uses the exact version of Soplex to solve the given LP. */
LP_Solution<Rational>
Solver::solve(const Matrix<Rational>& Inequalities, const Matrix<Rational>& Equations,
              const Vector<Rational>& Objective, bool maximize, const Set<Int>& initial_basis) const
{
   const Int dim = Inequalities.cols(), num_constr = Inequalities.rows() + Equations.rows();
   if (dim > std::numeric_limits<int>::max() || num_constr > std::numeric_limits<int>::max())
      throw std::runtime_error("Problem is too big for soplex");

   const int n = int(dim-1); // beware of leading constant term
   const int m = int(num_constr);

   // create soplex instance
   soplex::SoPlex soplex;

   // set parameters to activate rational solver
   soplex.setRealParam(soplex::SoPlex::RealParam::FEASTOL, 0.0);
   soplex.setRealParam(soplex::SoPlex::RealParam::OPTTOL, 0.0);
   soplex.setIntParam(soplex::SoPlex::IntParam::SOLVEMODE, soplex::SoPlex::SOLVEMODE_RATIONAL);
   soplex.setIntParam(soplex::SoPlex::IntParam::SYNCMODE, soplex::SoPlex::SYNCMODE_AUTO);
#if 0
   soplex.setIntParam(soplex::SoPlex::IntParam::READMODE, soplex::SoPlex::READMODE_RATIONAL);
   soplex.setIntParam(soplex::SoPlex::IntParam::CHECKMODE, soplex::SoPlex::CHECKMODE_RATIONAL);
#endif

   // turn off output
   soplex.setIntParam(soplex::SoPlex::IntParam::VERBOSITY, soplex::SoPlex::VERBOSITY_ERROR);
#if POLYMAKE_DEBUG
   if (debug_print) {
      soplex.setIntParam(soplex::SoPlex::IntParam::VERBOSITY, soplex::SoPlex::VERBOSITY_NORMAL);
   }
#endif

   // set objective sense
   soplex.setIntParam(soplex::SoPlex::IntParam::OBJSENSE,
                      maximize ? soplex::SoPlex::OBJSENSE_MAXIMIZE : soplex::SoPlex::OBJSENSE_MINIMIZE);

   // get value for infinity
   const double realinfty = soplex.realParam(soplex::SoPlex::RealParam::INFTY);
   const Rational plusinfinity(realinfty);
   const Rational minusinfinity(-realinfty);

   if (Equations.rows() > 0 && n != Equations.cols()-1) {
      throw std::runtime_error("Dimensions do not fit.\n");
   }

   // create variables/columns
   mpq_t* obj = new mpq_t [n];
   mpq_t* lower = new mpq_t [n];
   mpq_t* upper = new mpq_t [n];

   for (int j = 0; j < n; ++j) {
      mpq_init(obj[j]);
      mpq_set(obj[j], Objective[j+1].get_rep());

      mpq_init(lower[j]);
      mpq_init(upper[j]);
      mpq_set(lower[j], minusinfinity.get_rep());
      mpq_set(upper[j], plusinfinity.get_rep());
   }

   // add columns
   soplex.addColsRational(obj, lower, nullptr, nullptr, nullptr, nullptr, n, 0, upper);

   // free space
   for (int j = 0; j < n; ++j) {
      mpq_clear(lower[j]);
      mpq_clear(upper[j]);
      mpq_clear(obj[j]);
   }

   delete [] upper;
   delete [] lower;
   delete [] obj;

   mpq_t lhs;
   mpq_t rhs;
   mpq_t tmp;
   mpq_init(rhs);
   mpq_init(lhs);
   mpq_init(tmp);

   // create rows - inequalities
   for (int i = 0; i < Inequalities.rows(); ++i) {
      soplex::DSVectorRational row(0);
      for (int j = 0; j < n; ++j) {
         const Rational& val = Inequalities(i, j+1);
         if (!is_zero(val)) {
            mpq_set(tmp, val.get_rep());
            row.add(j, tmp);
         }
      }

      Rational val = -Inequalities(i, 0);
      mpq_set(rhs, plusinfinity.get_rep());
      mpq_set(lhs, val.get_rep());

      soplex.addRowRational(soplex::LPRowRational(lhs, row, rhs));
   }

   // create rows - equations
   for (int i = 0; i < Equations.rows(); ++i) {
      soplex::DSVectorRational row(0);
      for (int j = 0; j < n; ++j) {
         Rational val = Equations(i, j+1);
         if (!is_zero(val)) {
            mpq_set(tmp, val.get_rep());
            row.add(j, tmp);
         }
      }

      Rational val = -Equations(i, 0);
      mpq_set(lhs, val.get_rep());
      mpq_set(rhs, val.get_rep());

      soplex.addRowRational(soplex::LPRowRational(lhs, row, rhs));
   }
   mpq_clear(lhs);
   mpq_clear(rhs);
   mpq_clear(tmp);

#if POLYMAKE_DEBUG
   if (debug_print) {
      std::cout << "Number of columns: " << soplex.numColsRational() << std::endl;
      std::cout << "Number of rows: " << soplex.numRowsRational() << std::endl;
   }
#endif

   // add start basis
   if (!initial_basis.empty()) {
      soplex::SPxSolver::VarStatus* rows = new soplex::SPxSolver::VarStatus [m];
      soplex::SPxSolver::VarStatus* cols = new soplex::SPxSolver::VarStatus [n];

      // init basis
      for (int j = 0; j < n; ++j)
         cols[j] = soplex::SPxSolver::VarStatus::ZERO;

      for (int i = 0; i < m; ++i)
         rows[i] = soplex::SPxSolver::VarStatus::BASIC;

      int count = 0;
      for (const Int i : initial_basis) {
         rows[i] = soplex::SPxSolver::VarStatus::ON_LOWER;
         if (++count >= n)
            break;
      }
      soplex.setBasis(rows, cols);

      delete [] cols;
      delete [] rows;
   }

#if POLYMAKE_DEBUG
   if (debug_print) {
      soplex.writeFileRational("debug.lp", nullptr, nullptr, nullptr);
      std::cout << "Wrote debug output of LP to 'debug.lp'." << std::endl;
   }
#endif

   LP_Solution<Rational> result;

   // solves the LP
   const soplex::SPxSolver::Status status = soplex.solve();

   switch (status) {
   case soplex::SPxSolver::ERROR:
   case soplex::SPxSolver::NO_RATIOTESTER:
   case soplex::SPxSolver::NO_PRICER:
   case soplex::SPxSolver::NO_SOLVER:
   case soplex::SPxSolver::NOT_INIT:
   case soplex::SPxSolver::NO_PROBLEM:
   case soplex::SPxSolver::UNKNOWN:
      throw std::runtime_error("Error in setting up soplex.");

   case soplex::SPxSolver::ABORT_TIME:
   case soplex::SPxSolver::ABORT_ITER:
   case soplex::SPxSolver::ABORT_VALUE:
   case soplex::SPxSolver::REGULAR:
   case soplex::SPxSolver::RUNNING:
      throw std::runtime_error("Error in solving LP with soplex. This should not happen.");

   case soplex::SPxSolver::SINGULAR:
   case soplex::SPxSolver::ABORT_CYCLING:
      throw std::runtime_error("Numerical problems while solving LP with soplex.");

   case soplex::SPxSolver::OPTIMAL:
   {
      result.status = LP_status::valid;
      // get solution
      auto soplexsol = std::make_unique<mpq_t[]>(n+1);
      for (int j = 0; j <= n; ++j)
         mpq_init(soplexsol[j]);
      mpq_set_si(soplexsol[0], 1, 1);
      soplex.getPrimalRational(&soplexsol[1], n);

      // convert to polymake rationals;
      // add constant term from objective function
#if SOPLEX_VERSION_MAJOR >= 6
      result.objective_value = Rational(std::move(soplex.objValueRational().backend().data())) + Objective[0];
#else
      result.objective_value = Rational(std::move(soplex.objValueRational().getMpqRef())) + Objective[0];
#endif
      result.solution = Vector<Rational>(n+1, enforce_movable_values(&soplexsol[0]));
      break;
   }
   case soplex::SPxSolver::UNBOUNDED:
      result.status = LP_status::unbounded;
      break;

   case soplex::SPxSolver::INFEASIBLE:
      result.status = LP_status::infeasible;
      break;

   case soplex::SPxSolver::INForUNBD:
      // not sure what to do ...
      result.status = LP_status::infeasible;
      break;

   default:
      throw std::runtime_error("Unknown error.");
   }

   return result;
}

} } }

// -Wno-zero-as-null-pointer-constant
#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
