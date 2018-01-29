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

#ifndef POLYMAKE_MATRIX_LINALG_H
#define POLYMAKE_MATRIX_LINALG_H

#include "polymake/SparseMatrix.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/internal/linalg_exceptions.h"
#include "polymake/internal/sparse_linalg.h"


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
typename std::enable_if<is_field<E>::value, Matrix<E>>::type
solve_right(const GenericMatrix<TMatrix1, E>& A, const GenericMatrix<TMatrix2, E>& B)
{
   if (POLYMAKE_DEBUG || !Unwary<TMatrix1>::value || !Unwary<TMatrix2>::value) {
      if (B.rows() != A.rows())
         throw std::runtime_error("solve_right - mismatch in number of rows");
   }
   const int n(A.rows()), d(A.cols()), e(B.cols());
   SparseMatrix<E> A_aug(n*e, d*e);
   Vector<E> rhs(n*e);
   auto rhs_it(entire(rhs));
   for (int i=0; i<n; ++i) {
      for (int j=0; j<e; ++j, ++rhs_it) {
         A_aug.minor(scalar2set(i*e+j), sequence(d*j, d)) = A.minor(scalar2set(i),All);
         *rhs_it = B[i][j];
      }
   }
   return T(Matrix<E>(e, d, entire(lin_solve<E,false>(A_aug, rhs))));
}

   // solve the matrix equation X A = B for X by reducing it to A^T X^T = B^T
   
template <typename TMatrix1, typename TMatrix2, typename E>
typename std::enable_if<is_field<E>::value, Matrix<E>>::type
solve_left(const GenericMatrix<TMatrix1, E>& A, const GenericMatrix<TMatrix2, E>& B)
{
   return T(solve_right(T(A), T(B)));
}

} // end namespace pm

#endif // POLYMAKE_MATRIX_LINALG_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
