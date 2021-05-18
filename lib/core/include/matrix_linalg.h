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

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#endif

#include "boost/numeric/ublas/lu.hpp"

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#include "polymake/SparseMatrix.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/internal/linalg_exceptions.h"
#include "polymake/internal/sparse_linalg.h"
#include "polymake/internal/dense_linalg.h"

namespace pm {

/*

  We solve the matrix equation AX=B for X by transforming the system

   ____d___              ___e____
  |  a1    |             |  b1  |
  |  a2    |  ___e___    |  b2  |
  |        |  |     |    |      |
 n|        |  |x1 xe| =  |      | 
  |        |  d     |    |      |
  |        |  |_____|    |      |
  |  an    |             |  bn  |  
  |________|             |______|

into

  -a1-  0    0      |      b11
   0   -a1-  0      x1     b12
   0    0   -a1-    |      b1e
  -a2-  0    0      |      |
   0   -a2-  0             b2
   0    0   -a2-    |   =  |
        
  -an-  0    0      |      |
   0   -an-  0      xe     bn
   0    0   -an-    |      |
*/

template <typename TMatrix1, typename TMatrix2, typename E>
std::enable_if_t<is_field<E>::value, std::pair<SparseMatrix<E>, Vector<E>>>
augmented_system(const GenericMatrix<TMatrix1, E>& A, const GenericMatrix<TMatrix2, E>& B)
{
   const Int n(A.rows()), d(A.cols()), e(B.cols());
   SparseMatrix<E> A_aug(n*e, d*e);
   Vector<E> rhs(n*e);
   auto rhs_it = rhs.begin();
   for (Int i = 0; i < n; ++i) {
      for (Int j = 0; j < e; ++j, ++rhs_it) {
         A_aug.minor(scalar2set(i*e+j), sequence(d*j, d)) = A.minor(scalar2set(i),All);
         *rhs_it = B[i][j];
      }
   }
   return std::make_pair(A_aug, rhs);
}

template <typename TMatrix1, typename TMatrix2, typename E>
std::enable_if_t<is_field<E>::value, Matrix<E>>
solve_right(const GenericMatrix<TMatrix1, E>& A, const GenericMatrix<TMatrix2, E>& B)
{
   if (POLYMAKE_DEBUG || is_wary<TMatrix1>() || is_wary<TMatrix2>()) {
      if (B.rows() != A.rows())
         throw std::runtime_error("solve_right - mismatch in number of rows");
   }
   const auto aug_rhs(augmented_system(A,B));
   return T(Matrix<E>(B.cols(), A.cols(), lin_solve<E,false>(aug_rhs.first, aug_rhs.second).begin()));
}

/*

  In the floating-point case, to solve the matrix equation AX = B for X, we use the LU factorization provided by boost. 
  By expressing A as a product A = LU of a Lower and an Upper triangular matrix, we can find the solution to

      A X = L U X = B

  via

      L Y = B  // solve for Y
      U X = Y  // solve for X
 
  In fact, we use partial pivoting, so the LU decomposition reads  PA = LU,  so to solve  AX = B  we instead solve

      P A X  =  L U X  =  P B
  
  via

      L Y  =  P B
      U X  =  Y
    
  Because boost::ublas's LU solver only works for square matrices (even if this is not documented), 
  we need additional steps to process rectangular matrices.

  (1) If A has more rows than columns, instead of solving

        A X  =  B

      we solve 

        A^T A X == A^T B

      by LU-decomposing the (small) matrix A^T A. Numerically, it would be better to use QR decomposition here, see
      https://en.wikipedia.org/wiki/Moore%E2%80%93Penrose_inverse#The_QR_method ,
      but boost::ublas also doesn't provide this.

  (2) The case where A has more columns than rows needs SVD decomposition; this is easy implement and will be done when the need arises.

 */
   
template <typename TMatrix1, typename TMatrix2>
Matrix<double>
solve_right(const GenericMatrix<TMatrix1, double>& A, const GenericMatrix<TMatrix2, double>& B)
{
   if (POLYMAKE_DEBUG || is_wary<TMatrix1>() || is_wary<TMatrix2>()) {
      if (B.rows() != A.rows())
         throw std::runtime_error("solve_right: mismatch in number of rows");
   }
   if (A.cols() > A.rows()) {
      throw std::runtime_error("solve_right: the case A.cols() > A.rows() is not implemented yet.");
   }

   const bool square( A.cols() == A.rows() );
   
   const Int
      Arows ( square ? A.rows() : A.cols() ),
      Brows ( square ? B.rows() : A.cols() );

   boost::numeric::ublas::matrix<double> ublasA (Arows, A.cols());
   if (square)
      copy_range(entire(concat_rows(Matrix<double>(A))), ublasA.data().begin());
   else
      copy_range(entire(concat_rows(Matrix<double>(T(A)*A))), ublasA.data().begin());

   boost::numeric::ublas::matrix<double> ublasB (Brows, B.cols());
   if (square)
      copy_range(entire(concat_rows(Matrix<double>(B))), ublasB.data().begin());
   else
      copy_range(entire(concat_rows(Matrix<double>(T(A)*B))), ublasB.data().begin());

   boost::numeric::ublas::permutation_matrix<> ublasP(Arows); // permutation matrix for LU factorization
   boost::numeric::ublas::lu_factorize(ublasA, ublasP); // now ublasA is factored in-place into L and U
   boost::numeric::ublas::lu_substitute(ublasA, ublasP, ublasB); // now ublasB contains the solution
   
   Matrix<double> sol(Brows, B.cols());
   for (Int i = 0; i < Brows; ++i)
      for (Int j = 0; j < B.cols(); ++j) {
         const double b = ublasB(i,j);
         sol(i,j) = fabs(b) < 10.0 * std::numeric_limits<double>::epsilon()
                              ? 0
                              : b;
      }
   
   return sol;
}

// solve the matrix equation X A = B for X by reducing it to A^T X^T = B^T
   
template <typename TMatrix1, typename TMatrix2, typename E>
std::enable_if_t<is_field<E>::value, Matrix<E>>
solve_left(const GenericMatrix<TMatrix1, E>& A, const GenericMatrix<TMatrix2, E>& B)
{
   return T(solve_right(T(A), T(B)));
}

} // end namespace pm


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
