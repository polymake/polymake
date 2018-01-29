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

#include "polymake/polytope/soplex_interface.h"
#include "polymake/Rational.h"

#define SOPLEX_WITH_GMP
#include "soplex.h"

namespace polymake { namespace polytope { namespace soplex_interface {


/** This method uses the exact version of Soplex to solve the given LP. */
solver::lp_solution
solver::solve_lp(const Matrix<Rational>& Inequalities, const Matrix<Rational>& Equations,
                 const Vector<Rational>& Objective, bool maximize)
{
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
   if ( debug_print )
   {
      soplex.setIntParam(soplex::SoPlex::IntParam::VERBOSITY, soplex::SoPlex::VERBOSITY_NORMAL);
   }
#endif

   // set objective sense
   if ( maximize )
      soplex.setIntParam(soplex::SoPlex::IntParam::OBJSENSE, soplex::SoPlex::OBJSENSE_MAXIMIZE);
   else
      soplex.setIntParam(soplex::SoPlex::IntParam::OBJSENSE, soplex::SoPlex::OBJSENSE_MINIMIZE);

   // get value for infinity
   const double realinfty = soplex.realParam(soplex::SoPlex::RealParam::INFTY);
   const Rational plusinfinity(realinfty);
   const Rational minusinfinity(-realinfty);

   const int n(Inequalities.cols() - 1); // beware of leading constant term

   if ( Equations.rows() > 0 && n != Equations.cols() - 1 )
   {
      throw std::runtime_error("Dimensions do not fit.\n");
   }

   // create variables/columns
   mpq_t* obj = new mpq_t [n];
   mpq_t* lower = new mpq_t [n];
   mpq_t* upper = new mpq_t [n];

   for (int j = 0; j < n; ++j)
   {
      mpq_init(obj[j]);
      mpq_set(obj[j], Objective[j+1].get_rep());

      mpq_init(lower[j]);
      mpq_init(upper[j]);
      mpq_set(lower[j], minusinfinity.get_rep());
      mpq_set(upper[j], plusinfinity.get_rep());
   }

   // add columns
   soplex.addColsRational(obj, lower, 0, 0, 0, 0, n, 0, upper);

   // free space
   for (int j = 0; j < n; ++j)
   {
      mpq_clear(lower[j]);
      mpq_clear(upper[j]);
      mpq_clear(obj[j]);
   }

   delete [] upper;
   delete [] lower;
   delete [] obj;

   // reserve space for rows
   mpq_t* rowValues = new mpq_t [n];
   int* rowIndices = new int [n];

   for (int j = 0; j < n; ++j)
      mpq_init(rowValues[j]);

   mpq_t lhs;
   mpq_t rhs;
   mpq_init(rhs);
   mpq_init(lhs);

   // create rows - inequalities
   for (int i = 0; i < Inequalities.rows(); ++i)
   {
      int cnt = 0;
      for (int j = 0; j < n; ++j)
      {
         Rational val = Inequalities(i,j+1);
         if ( ! is_zero(val) )
         {
            mpq_set(rowValues[cnt], val.get_rep());
            rowIndices[cnt++] = j;
         }
      }

      Rational val = -Inequalities(i,0);
      mpq_set(rhs, plusinfinity.get_rep());
      mpq_set(lhs, val.get_rep());

      soplex.addRowRational(&lhs, rowValues, rowIndices, cnt, &rhs);
   }

   // create rows - equations
   for (int i = 0; i < Equations.rows(); ++i)
   {
      int cnt = 0;
      for (int j = 0; j < n; ++j)
      {
         Rational val = Equations(i,j+1);
         if ( ! is_zero(val) )
         {
            // mpq_clear(obj[j]);
            mpq_set(rowValues[cnt], val.get_rep());
            rowIndices[cnt++] = j;
         }
      }

      Rational val = -Equations(i,0);
      mpq_set(lhs, val.get_rep());
      mpq_set(rhs, val.get_rep());

      soplex.addRowRational(&lhs, rowValues, rowIndices, cnt, &rhs);
   }
   mpq_clear(lhs);
   mpq_clear(rhs);

   for (int j = 0; j < n; ++j)
      mpq_clear(rowValues[j]);

   delete [] rowValues;
   delete [] rowIndices;

#if POLYMAKE_DEBUG
   if ( debug_print )
   {
      std::cout << "Number of columns: " << soplex.numColsRational() << std::endl;
      std::cout << "Number of rows: " << soplex.numRowsRational() << std::endl;
   }
#endif

   // add start basis
   if ( ! initial_basis.empty() )
   {
      int m(Inequalities.rows() + Equations.rows());

      soplex::SPxSolver::VarStatus* rows = new soplex::SPxSolver::VarStatus [m];
      soplex::SPxSolver::VarStatus* cols = new soplex::SPxSolver::VarStatus [n];

      // init basis
      for (int j = 0; j < n; ++j)
         cols[j] = soplex::SPxSolver::VarStatus::ZERO;

      for (int i = 0; i < m; ++i)
         rows[i] = soplex::SPxSolver::VarStatus::BASIC;

      int count = 0;
      for (Entire< Set<int> >::const_iterator it = entire(initial_basis); ! it.at_end(); ++it)
      {
         rows[*it] = soplex::SPxSolver::VarStatus::ON_LOWER;
         if ( ++count >= n )
            break;
      }
      soplex.setBasis(rows, cols);

      delete [] cols;
      delete [] rows;
   }

#if POLYMAKE_DEBUG
   if ( debug_print )
   {
      soplex.writeFileRational("debug.lp", 0, 0, 0);
      std::cout << "Wrote debug output of LP to 'debug.lp'." << std::endl;
   }
#endif

   // solves the LP
   soplex::SPxSolver::Status status = soplex.solve();

   switch ( status )
   {
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
      // get solution
      auto soplexsol=std::make_unique<mpq_t[]>(n+1);
      for (int j = 0; j <= n; ++j)
         mpq_init(soplexsol[j]);
      mpq_set_si(soplexsol[0], 1, 1);
      soplex.getPrimalRational(&soplexsol[1], n);

      // convert to polymake rationals;
      // add constant term from objective funtion
      return lp_solution(Rational(std::move(soplex.objValueRational().getMpqRef())) + Objective[0],
                         Vector<Rational>(n+1, enforce_movable_values(&soplexsol[0])));
   }
   case soplex::SPxSolver::UNBOUNDED:
      throw unbounded();

   case soplex::SPxSolver::INFEASIBLE:
      throw infeasible();

   case soplex::SPxSolver::INForUNBD:
      // not sure what to do ...
      throw infeasible();

   default:
      throw std::runtime_error("Unknown error.");
   }
}

void solver::set_basis(const Set<int>& basis)
{
   initial_basis = basis;
}


} } }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
