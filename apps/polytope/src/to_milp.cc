/* Copyright (c) 1997-2019
   Ewgenij Gawrilow, Michael Joswig, and the polymake team
   Technische Universität Berlin, Germany
   Thomas Opfer
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
#include "polymake/Vector.h"
#include "polymake/Matrix.h"
#include "polymake/Integer.h"
#include "polymake/Rational.h"
#include "polymake/ListMatrix.h"
#include "polymake/Set.h"
#include "polymake/polytope/solve_LP.h"
#include "polymake/polytope/generic_milp_client.h"


#define TO_WITHOUT_DOUBLE
#define TO_DISABLE_OUTPUT
#include "TOSimplex/TOExMipSol.h"
#include "polymake/common/TOmath_decl.h"

namespace polymake { namespace polytope {

   namespace to_interface {

      template<typename Scalar>
      TOExMipSol::MIP<Scalar> construct_mip(const Matrix<Scalar>& F, const Matrix<Scalar>& A, bool maximize, std::vector<TOExMipSol::rowElement<Scalar>>& objfunc, std::vector<char>& numbersystems) {
         // FIXME check: only nonzero?
      	unsigned int numCols = F.cols()-1;	// number of columns/variables

      	TOExMipSol::MIP<Scalar> mip;

         // Initializing important mip attributes
         mip.linf = std::vector<bool>( numCols, true );	// true: variable i has no lower bound, false: variable i has lower bound
         mip.uinf = std::vector<bool>( numCols, true );	// true: variable i has no upper bound, false: variable i has upper bound
         mip.lbounds = std::vector<Scalar>( numCols, 0 );	// lower bound for variable i (if any)
         mip.ubounds = std::vector<Scalar>( numCols, 0 );	// upper bound for variable i (if any)
         mip.numbersystems = numbersystems;	// variable i is: G: Integer, B: Binary, R: Real
         mip.objfunc = objfunc;	// objective function (sparse)
         mip.matrix = std::vector<TOExMipSol::constraint<Scalar> >();	// constraints (sparse)
         mip.varNames = std::vector<std::string>( numCols );	// names of the variables
      	mip.maximize = maximize;	// true: maximize, false: minimize

      	// Temporary variables
         TOExMipSol::constraint<Scalar> row;
         TOExMipSol::rowElement<Scalar> element;

         for (int i = 0; i < F.rows(); ++i) {
            row.rhs = F(i,0);
            row.type = -1;	// -1: <=, 0: =, 1: >=
            row.constraintElements = std::vector<TOExMipSol::rowElement<Scalar> >();

            for (int j = 1; j < F.cols(); ++j) {
               if ( F(i,j) != 0 ) {
                  element.index = j-1;
                  element.mult = -F(i,j);
                  row.constraintElements.push_back( element );
               }
            }
            // Add constraint
            mip.matrix.push_back( row );
         }
         
         for (int i = 0; i < A.rows(); ++i) {
            row.rhs = A(i,0);
            row.type = 0;	// -1: <=, 0: =, 1: >=
            row.constraintElements = std::vector<TOExMipSol::rowElement<Scalar> >();

            for (int j = 1; j < A.cols(); ++j) {
               if ( A(i,j) != 0 ) {
                  element.index = j-1;
                  element.mult = -A(i,j);
                  row.constraintElements.push_back( element );
               }
            }
            // Add constraint
            mip.matrix.push_back( row );
         }

         // Maximization problem
         mip.maximize = maximize;
         return mip;
      }

      template <typename Scalar>
      Matrix<Integer> to_compute_lattice_points ( Matrix<Scalar>& F, Matrix<Scalar>& A) {
         
      	unsigned int numCols = F.cols()-1;	// number of columns/variables
         std::vector<char> numbersystems( numCols, 'G' );	// variable i is: G: Integer, B: Binary, R: Real
         std::vector<TOExMipSol::rowElement<Scalar> > objfunc;	// objective function (sparse)
         TOExMipSol::MIP<Scalar> mip = construct_mip(F, A, true, objfunc, numbersystems);
         // Solver
         TOExMipSol::TOMipSolver<Scalar> solver;

         // Results go here
         Scalar objval;
         std::vector<Scalar> assignment;
         std::vector<std::vector<Scalar> > allAssignments;

         // Solve
         // second argument true: search all integer feasible solutions, false: only search optimal solution
         // last argument: contains all solutions discovered during solution process, can be NULL
         typename TOExMipSol::TOMipSolver<Scalar>::solstatus stat = solver.solve( mip, true, objval, assignment, &allAssignments );

      	if( stat != TOExMipSol::TOMipSolver<Scalar>::OPTIMAL && stat != TOExMipSol::TOMipSolver<Scalar>::INFEASIBLE && stat != TOExMipSol::TOMipSolver<Scalar>::INForUNB ) {
            throw std::runtime_error("unbounded polyhedron or computation failed");
         }

      	if( stat == TOExMipSol::TOMipSolver<Scalar>::INFEASIBLE || stat == TOExMipSol::TOMipSolver<Scalar>::INForUNB ) {
            return Matrix<Integer>(0,numCols+1);
         }

         ListMatrix<Vector<Integer> > L;
         for ( unsigned int i = 0; i < allAssignments.size(); ++i ) {
            Vector<Integer> V(numCols,allAssignments[i].begin());
            L /= V;
         }
         return ones_vector<Integer>(L.rows())|L;
      }

   template<typename Scalar>
      class MILP_SolverImpl : public MILP_Solver<Scalar> {
         public: 
            MILP_SolverImpl() {}

            MILP_Solution<Scalar> solve(const Matrix<Scalar>& H,
                  const Matrix<Scalar>& E,
                  const Vector<Scalar>& Obj,
                  const Set<int>& integerVariables,
                  bool maximize) const {
               std::vector<char> numbersystems( Obj.dim()-1, 'R' );	// variable i is: G: Integer, B: Binary, R: Real
               for(const auto& i: integerVariables){
                  if(i != 0) numbersystems[i-1] = 'G';
               }

               std::vector<TOExMipSol::rowElement<Scalar> > objfunc;	// objective function (sparse)
               TOExMipSol::rowElement<Scalar> element;
               for (int i=1; i<Obj.dim(); i++) {
                  if ( Obj[i] != 0 ) {
                     element.index = i-1;
                     element.mult = Obj[i];
                     objfunc.push_back( element );
                  }
               }
               auto TOMIP = construct_mip<Scalar>(H, E, maximize, objfunc, numbersystems);
               /////////////////////////////////////////////////////////////////////////
               TOExMipSol::TOMipSolver<Scalar> solver;

               // Results go here
               std::vector<Scalar> assignment;
               MILP_Solution<Scalar> result;


               // Solve
               // second argument true: search all integer feasible solutions, false: only search optimal solution
               // last argument: contains all solutions discovered during solution process, can be NULL
               typename TOExMipSol::TOMipSolver<Scalar>::solstatus stat = solver.solve( TOMIP, false, result.objective_value, assignment, NULL );

               if( stat != TOExMipSol::TOMipSolver<Scalar>::OPTIMAL && stat != TOExMipSol::TOMipSolver<Scalar>::INFEASIBLE && stat != TOExMipSol::TOMipSolver<Scalar>::INForUNB ) {
                  throw std::runtime_error("unbounded polyhedron or computation failed");
               }

               if( stat == TOExMipSol::TOMipSolver<Scalar>::INFEASIBLE ) {
                  result.status = LP_status::infeasible;
               } else if( stat == TOExMipSol::TOMipSolver<Scalar>::INForUNB ) {
                  result.status = LP_status::infeasibleOrUnbounded;
               } else {
                  result.status = LP_status::valid;
                  result.objective_value += Obj[0];
                  Vector<Rational> solution(Obj.dim()-1, assignment.begin());
                  result.solution = ones_vector<Rational>(1) | solution;
               }
               return result;
            }
      };

      template<typename Scalar>
      auto create_MILP_solver()
      {
         return cached_MILP_solver<Scalar>(new MILP_SolverImpl<Scalar>(), true);
      }
   
   } // namespace to_interface

   template<typename Scalar>
   Matrix<Integer> to_lattice_points(perl::Object p) {
      Matrix<Scalar> F = p.give("FACETS|INEQUALITIES");
      Matrix<Scalar> A = p.lookup("AFFINE_HULL|EQUATIONS");
      return to_interface::to_compute_lattice_points(F,A);
   }
   
   template<typename Scalar>
   void to_milp_client(perl::Object p, perl::Object milp, bool maximize) {
      to_interface::MILP_SolverImpl<Scalar> S;
      generic_milp_client<Scalar, to_interface::MILP_SolverImpl<Scalar>>(p, milp, maximize, S);
   }
   
   FunctionTemplate4perl("to_lattice_points<Scalar>(Polytope<Scalar>)");
   
   FunctionTemplate4perl("to_milp_client<Scalar>(Polytope<Scalar>, MixedIntegerLinearProgram<Scalar>, $)");
   

   InsertEmbeddedRule("function to.milp: create_MILP_solver<Scalar> () : c++ (name => 'to_interface::create_MILP_solver') : returns(cached);\n");

} // namespace polytope
} // namespace polymake

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
