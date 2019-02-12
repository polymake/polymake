/* Copyright (c) 1997-2018
   Ewgenij Gawrilow, Michael Joswig (Technische Universitaet Berlin, Germany)
   Thomas Opfer
   http://www.polymake.org

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version: http://www.gnu.org/licenses/gpl.txt.

   This program is distributed in the hope that it will be useful,
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


#define TO_WITHOUT_DOUBLE
#define TO_DISABLE_OUTPUT
#include "TOSimplex/TOExMipSol.h"
#include "polymake/common/TOmath_decl.h"

namespace polymake { namespace polytope {

   namespace {

      template <typename Scalar>
      Matrix<Integer> to_compute_lattice_points ( Matrix<Scalar>& F, Matrix<Scalar>& A) {
         
         // FIXME check: only nonzero?
      	unsigned int numCols = F.cols()-1;	// number of columns/variables

      	TOExMipSol::MIP<Scalar> mip;

         // Initializing important mip attributes
         mip.linf = std::vector<bool>( numCols, true );	// true: variable i has no lower bound, false: variable i has lower bound
         mip.uinf = std::vector<bool>( numCols, true );	// true: variable i has no upper bound, false: variable i has upper bound
         mip.lbounds = std::vector<Scalar>( numCols, 0 );	// lower bound for variable i (if any)
         mip.ubounds = std::vector<Scalar>( numCols, 0 );	// upper bound for variable i (if any)
         mip.numbersystems = std::vector<char>( numCols, 'G' );	// variable i is: G: Integer, B: Binary, R: Real
         mip.objfunc = std::vector<TOExMipSol::rowElement<Scalar> >();	// objective function (sparse)
         mip.matrix = std::vector<TOExMipSol::constraint<Scalar> >();	// constraints (sparse)
         mip.varNames = std::vector<std::string>( numCols );	// names of the variables
      	mip.maximize = true;	// true: maximize, false: minimize

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
         mip.maximize = true;	// true: maximize, false: minimize

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
   }

   template<typename Scalar>
   Matrix<Integer> to_lattice_points(perl::Object p) {
      Matrix<Scalar> F = p.give("FACETS|INEQUALITIES");
      Matrix<Scalar> A = p.lookup("AFFINE_HULL|EQUATIONS");
      return to_compute_lattice_points(F,A);
   }
   
   FunctionTemplate4perl("to_lattice_points<Scalar>(Polytope<Scalar>)");
   } 
}

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End: