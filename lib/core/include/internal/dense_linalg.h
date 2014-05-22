/* Copyright (c) 1997-2014
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

/** @file dense_linalg.h
    @brief Linear Algebra algorithms for dense vector and matrix types
 */

#ifndef POLYMAKE_INTERNAL_DENSE_LINALG_H
#define POLYMAKE_INTERNAL_DENSE_LINALG_H

#include "polymake/vector"
#include "polymake/Vector.h"

namespace pm {

/// determinant of a matrix
template <typename E>
typename enable_if<E, is_field<E>::value>::type
det(Matrix<E> M)
{
   const int dim=M.rows();
   if (!dim) return zero_value<E>();

   std::vector<int> row_index(dim);
   copy(entire(sequence(0,dim)), row_index.begin());
   E result = one_value<E>();

   for (int c=0; c<dim; ++c) {
      int r=c;
      while (is_zero(M(row_index[r],c))) {
         if (++r==dim) return zero_value<E>();
      }
      if (r!=c) {
         std::swap(row_index[r],row_index[c]);
         negate(result);
      }
      E *ppivot=&M(row_index[c],c);
      const E pivot=*ppivot;
      result*=pivot;
      E *e=ppivot;
      for (int i=c+1; i<dim; ++i) (*++e)/=pivot;
      for (++r; r<dim; ++r) {
         E *e2=&M(row_index[r],c);
         const E factor=*e2;
         if (!is_zero(factor)) {
            e=ppivot;
            for (int i=c+1; i<dim; ++i) (*++e2)-=(*++e)*factor;
         }
      }
   }
   return result;
}

template <typename E>
typename enable_if<Vector<E>, is_field<E>::value>::type
reduce(Matrix<E> M, Vector<E> V)
{
   const int n_rows=M.rows();
   const int n_cols=M.cols();

   std::vector<int> row_index(n_rows);
   copy(entire(sequence(0,n_rows)), row_index.begin());
   
   int row=0;
   for (int c=0; c<n_cols && row < n_rows; ++c) {
      //look for Pivot-Elmt.
      int r=row;
      while (r < n_rows && is_zero(M(row_index[r],c)))
         ++r;
      if (r==n_rows)
         continue;

      if (r!=row)
         std::swap(row_index[r],row_index[row]);

      E *ppivot=&M(row_index[row],c);
      const E pivot=*ppivot;
      E *e=ppivot;
      for (int i=c; i<n_cols; ++i,++e)
         (*e)/=pivot;

      for (++r; r<n_rows; ++r) {
         E *e2 = &M(row_index[r],c);
         const E factor=*e2;
         if (!is_zero(factor)) {
            e=ppivot;
            for (int i=c; i<n_cols; ++i,++e,++e2)
               (*e2)-=(*e)*factor;
         }
      }
      ++row;
   }

   int r=0; int c=0;
   for (typename Vector<E>::iterator vi = V.begin(); 
        r < n_rows && c < n_cols && vi != V.end();
        ++r,++c,++vi) {
      while (c < n_cols && is_zero(M(row_index[r],c))) {
         ++c; ++vi;
      }
      if (c == n_cols) break;

      E *e = &M(row_index[r],c);
      const E factor = *vi;
      if (!is_zero(*vi)) {
         for (typename Vector<E>::iterator ui = vi; ui != V.end(); ++ui, ++e)
            *ui -= (*e)*factor;
      }
   }
   return V;
}

/// matrix inversion
template <typename E>
typename enable_if<Matrix<E>, is_field<E>::value>::type
inv(Matrix<E> M)
{
   const int dim=M.rows();
   std::vector<int> row_index(dim);
   copy(entire(sequence(0,dim)), row_index.begin());
   Matrix<E> u=unit_matrix<E>(dim);

   for (int c=0; c<dim; ++c) {
      int r=c;
      while (is_zero(M(row_index[r],c))) {
         if (++r==dim) throw degenerate_matrix();
      }
      E *ppivot=&M(row_index[r],c);
      const E pivot=*ppivot;
      E *urow=&u(row_index[r],0);
      if (r!=c) std::swap(row_index[r],row_index[c]);
      if (!is_one(pivot)) {
         E *e=ppivot;
         for (int i=c+1; i<dim; ++i) (*++e)/=pivot;
         for (int i=0; i<=c; ++i) urow[row_index[i]]/=pivot;
      }
      for (r=0; r<dim; ++r) {
         if (r==c) continue;
         E *e2=&M(row_index[r],c);
         const E factor=*e2;
         if (!is_zero(factor)) {
            E *e=ppivot;
            for (int i=c+1; i<dim; ++i) (*++e2)-=(*++e)*factor;
            E *urow2=&u(row_index[r],0);
            for (int i=0; i<=c; ++i) urow2[row_index[i]]-=urow[row_index[i]]*factor;
         }
      }
   }
   return Matrix<E>(dim, dim, select(rows(u),row_index).begin());
}

Matrix<double> inv(Matrix<double> M);

/// solving systems of linear equations
template <typename E>
typename enable_if<Vector<E>, is_field<E>::value>::type
lin_solve(Matrix<E> A, Vector<E> B)
{
   const int m=A.rows(), n=A.cols();
   if (m<n) throw degenerate_matrix();
   std::vector<int> row_index(m);
   copy(entire(sequence(0,m)), row_index.begin());

   for (int c=0; c<n; ++c) {
      int r=c;
      while (is_zero(A(row_index[r],c))) {
         if (++r==m) throw degenerate_matrix();
      }
      E *ppivot=&A(row_index[r],c);
      const E pivot=*ppivot;
      if (r!=c) std::swap(row_index[r],row_index[c]);
      r=row_index[c];
      if (!is_one(pivot)) {
         E *e=ppivot;
         for (int i=c+1; i<n; ++i) (*++e) /= pivot;
         B[r] /= pivot;
      }
      for (int c2=c+1; c2<m; ++c2) {
         const int r2=row_index[c2];
         E *e2=&A(r2,c);
         const E factor=*e2;
         if (!is_zero(factor)) {
            E *e=ppivot;
            for (int i=c+1; i<n; ++i) (*++e2) -= (*++e) * factor;
            B[r2] -= B[r] * factor;
         }
      }
   }
   for (int c=n; c<m; ++c) {
      if (!is_zero(B[row_index[c]])) throw infeasible();
   }

   Vector<E> x(n);
   for (int c=n-1; c>=0; --c) {
      x[c]=B[row_index[c]];
      for (int c2=0; c2<c; ++c2) {
         const int r2=row_index[c2];
         B[r2] -= x[c] * A(r2,c);
      }
   }

   return x;
}

Vector<double> lin_solve(Matrix<double> A, Vector<double> B);

} // end namespace pm

#endif // POLYMAKE_INTERNAL_DENSE_LINALG_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
