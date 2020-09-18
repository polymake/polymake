/* Copyright (c) 1997-2020
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

/** @file dense_linalg.h
    @brief Linear Algebra algorithms for dense vector and matrix types
 */

#ifndef POLYMAKE_INTERNAL_DENSE_LINALG_H
#define POLYMAKE_INTERNAL_DENSE_LINALG_H

#include "polymake/vector"
#include "polymake/Vector.h"
#include "polymake/GenericStruct.h"

namespace pm {

/// determinant of a matrix
template <typename E>
std::enable_if_t<is_field<E>::value, E>
det(Matrix<E> M)
{
   const Int dim = M.rows();

   // the case distinction by dim is organized such that higher dimensions are sorted out first
   if (dim>3) { // generic case: Gauss elimination
      E result = one_value<E>();
      std::vector<Int> row_index(dim);
      copy_range(entire(sequence(0, dim)), row_index.begin());
      
      for (Int c = 0; c < dim; ++c) {
         Int r = c;
         while (is_zero(M(row_index[r], c))) {
            if (++r == dim) return zero_value<E>();
         }
         if (r != c) {
            std::swap(row_index[r], row_index[c]);
            negate(result);
         }
         E *ppivot=&M(row_index[c],c);
         const E pivot=*ppivot;
         result*=pivot;
         E *e=ppivot;
         for (Int i = c+1; i < dim; ++i)
            (*++e) /= pivot;
         for (++r; r < dim; ++r) {
            E* e2 = &M(row_index[r], c);
            const E factor = *e2;
            if (!is_zero(factor)) {
               e = ppivot;
               for (Int i = c+1; i < dim; ++i)
                  (*++e2) -= (*++e) * factor;
            }
         }
      }
      return result;
   } else switch(dim) {
      case 3: // Laplace/first column: 9 multiplications + 5 additions (better than Sarrus)
         return M(0,0)*(M(1,1)*M(2,2)-M(1,2)*M(2,1)) - M(1,0)*(M(0,1)*M(2,2)-M(2,1)*M(0,2)) + M(2,0)*(M(0,1)*M(1,2)-M(1,1)*M(0,2));
      case 2: return M(0,0)*M(1,1) - M(1,0)*M(0,1);
      case 1: return M(0,0);
      default: return one_value<E>();
      }
}

template <typename E>
std::enable_if_t<is_field<E>::value, Vector<E>>
reduce(Matrix<E> M, Vector<E> V)
{
   const Int n_rows = M.rows();
   const Int n_cols = M.cols();

   std::vector<Int> row_index(n_rows);
   copy_range(entire(sequence(0, n_rows)), row_index.begin());
   
   Int row = 0;
   for (Int c = 0; c < n_cols && row < n_rows; ++c) {
      //look for Pivot-Elmt.
      Int r = row;
      while (r < n_rows && is_zero(M(row_index[r], c)))
         ++r;
      if (r == n_rows)
         continue;

      if (r != row)
         std::swap(row_index[r], row_index[row]);

      E* ppivot = &M(row_index[row], c);
      const E pivot = *ppivot;
      E* e = ppivot;
      for (Int i = c; i < n_cols; ++i,++e)
         (*e) /= pivot;

      for (++r; r < n_rows; ++r) {
         E* e2 = &M(row_index[r], c);
         const E factor = *e2;
         if (!is_zero(factor)) {
            e = ppivot;
            for (Int i = c; i < n_cols; ++i, ++e, ++e2)
               (*e2) -= (*e) * factor;
         }
      }
      ++row;
   }

   Int r = 0;
   Int c = 0;
   for (auto vi = V.begin(); 
        r < n_rows && c < n_cols && vi != V.end();
        ++r, ++c, ++vi) {
      while (c < n_cols && is_zero(M(row_index[r], c))) {
         ++c; ++vi;
      }
      if (c == n_cols) break;

      E* e = &M(row_index[r], c);
      const E factor = *vi;
      if (!is_zero(*vi)) {
         for (auto ui = vi; ui != V.end(); ++ui, ++e)
            *ui -= (*e)*factor;
      }
   }
   return V;
}

/// matrix inversion
template <typename E>
std::enable_if_t<is_field<E>::value, Matrix<E>>
inv(Matrix<E> M)
{
   const Int dim = M.rows();
   std::vector<Int> row_index(dim);
   copy_range(entire(sequence(0, dim)), row_index.begin());
   Matrix<E> u = unit_matrix<E>(dim);

   for (Int c = 0; c < dim; ++c) {
      Int r = c;
      while (is_zero(M(row_index[r], c))) {
         if (++r == dim) throw degenerate_matrix();
      }
      E* ppivot = &M(row_index[r], c);
      const E pivot = *ppivot;
      E* urow = &u(row_index[r], 0);
      if (r != c)
         std::swap(row_index[r], row_index[c]);
      if (!is_one(pivot)) {
         E* e = ppivot;
         for (Int i = c+1; i < dim; ++i)
            (*++e) /= pivot;
         for (Int i = 0; i <= c; ++i)
            urow[row_index[i]] /= pivot;
      }
      for (r = 0; r < dim; ++r) {
         if (r==c) continue;
         E* e2 = &M(row_index[r], c);
         const E factor = *e2;
         if (!is_zero(factor)) {
            E* e = ppivot;
            for (Int i = c+1; i < dim; ++i)
               (*++e2) -= (*++e) * factor;
            E* urow2 = &u(row_index[r], 0);
            for (Int i = 0; i <= c; ++i)
               urow2[row_index[i]] -= urow[row_index[i]] * factor;
         }
      }
   }
   return Matrix<E>(dim, dim, select(rows(u),row_index).begin());
}

Matrix<double> inv(Matrix<double> M);

/// solving systems of linear equations
template <typename E>
std::enable_if_t<is_field<E>::value, Vector<E>>
lin_solve(Matrix<E> A, Vector<E> b)
{
   const Int m = A.rows();
   const Int n = A.cols();
   if (m < n) throw degenerate_matrix();
   std::vector<Int> row_index(m);
   copy_range(entire(sequence(0, m)), row_index.begin());

   for (Int c = 0; c < n; ++c) {
      Int r = c;
      while (is_zero(A(row_index[r], c))) {
         if (++r == m) throw degenerate_matrix();
      }
      E* ppivot = &A(row_index[r], c);
      const E pivot = *ppivot;
      if (r != c)
         std::swap(row_index[r], row_index[c]);
      r = row_index[c];
      if (!is_one(pivot)) {
         E* e = ppivot;
         for (Int i = c+1; i < n; ++i)
            (*++e) /= pivot;
         b[r] /= pivot;
      }
      for (Int c2 = c+1; c2 < m; ++c2) {
         const Int r2 = row_index[c2];
         E* e2 = &A(r2, c);
         const E factor = *e2;
         if (!is_zero(factor)) {
            E* e = ppivot;
            for (Int i = c+1; i < n; ++i)
               (*++e2) -= (*++e) * factor;
            b[r2] -= b[r] * factor;
         }
      }
   }
   for (Int c = n; c < m; ++c) {
      if (!is_zero(b[row_index[c]]))
         throw infeasible();
   }

   Vector<E> x(n);
   for (Int c = n-1; c >= 0; --c) {
      x[c] = b[row_index[c]];
      for (Int c2 = 0; c2 < c; ++c2) {
         const Int r2 = row_index[c2];
         b[r2] -= x[c] * A(r2,c);
      }
   }

   return x;
}

class SingularValueDecomposition :
   public GenericStruct<SingularValueDecomposition> {
public:
   typedef Matrix<double> matrix_type;

   // sigma: input matrix converted to the diagonal form
   // input = left_companion * sigma * T(right_companion)
   DeclSTRUCT( DeclFIELD(sigma, matrix_type)
               DeclFIELD(left_companion, matrix_type)
               DeclFIELD(right_companion, matrix_type));
};



Vector<double> lin_solve(Matrix<double> A, Vector<double> b);
Vector<double> eigenvalues(Matrix<double> M);
Matrix<double> householder_trafo(const Vector<double>& v);
std::pair< Matrix<double>,Matrix<double> > qr_decomp(Matrix<double> M);
SingularValueDecomposition singular_value_decomposition(Matrix<double> M);
Matrix<double> moore_penrose_inverse(const Matrix<double>& M);

} // end namespace pm

namespace polymake {

using pm::SingularValueDecomposition;

}

#endif // POLYMAKE_INTERNAL_DENSE_LINALG_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
