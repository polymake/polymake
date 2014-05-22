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

#include <cmath>
#include "polymake/Matrix.h"
#include "polymake/linalg.h"
#include <algorithm>

namespace pm {

Matrix<double> inv(Matrix<double> m)
{
   const int dim=m.rows();
   std::vector<int> row_index(dim);
   copy(entire(sequence(0,dim)), row_index.begin());
   Matrix<double> u=unit_matrix<double>(dim);
   const double epsilon=1e-8;

   for (int c=0; c<dim; ++c) {
      int r=0;
      double max_pivot=0;
      for (int rr=c; rr<dim; ++rr) {
         const double p=abs(m(row_index[rr],c));
         if (p > max_pivot) {
            r=rr;
            max_pivot=p;
         }
      }
      if (max_pivot > epsilon) {
         double *ppivot=&m(row_index[r],c);
         const double pivot=*ppivot;
         double *urow=&u(row_index[r],0);
         if (r!=c) std::swap(row_index[r],row_index[c]);
         if (pivot != 1) {
            double *e=ppivot;
            for (int i=c+1; i<dim; ++i) (*++e)/=pivot;
            for (int i=0; i<=c; ++i) urow[row_index[i]]/=pivot;
         }
         for (r=0; r<dim; ++r) {
            if (r==c) continue;
            double *e2=&m(row_index[r],c);
            const double factor=*e2;
            if (abs(factor) > epsilon) {
               double *e=ppivot;
               for (int i=c+1; i<dim; ++i) (*++e2)-=(*++e)*factor;
               double *urow2=&u(row_index[r],0);
               for (int i=0; i<=c; ++i) urow2[row_index[i]]-=urow[row_index[i]]*factor;
            }
         }
      } else {
         throw degenerate_matrix();
      }
   }
   return Matrix<double>(dim, dim, select(rows(u),row_index).begin());
}


Vector<double> lin_solve(Matrix<double> A, Vector<double> B)
{
   const int m=A.rows(), n=A.cols();
   const double epsilon=1e-8;

   if (m < n) throw degenerate_matrix();
   std::vector<int> row_index(m);
   copy(entire(sequence(0,m)), row_index.begin());

   for (int c=0; c<n; ++c) {
      int r=0;
      double max_pivot=0;
      for (int rr=c; rr<m; ++rr) {
         const double p=abs(A(row_index[rr],c));
         if (p > max_pivot) {
            r=rr;
            max_pivot=p;
         }
      }
      if (max_pivot > epsilon) {
         double *ppivot=&A(row_index[r],c);
         const double pivot=*ppivot;
         if (r!=c) std::swap(row_index[r],row_index[c]);
         r=row_index[c];
         if (pivot != 1) {
            double *e=ppivot;
            for (int i=c+1; i<n; ++i) (*++e)/=pivot;
            B[r]/=pivot;
         }
         for (int c2=c+1; c2<m; ++c2) {
            const int r2=row_index[c2];
            double *e2=&A(r2,c);
            const double factor=*e2;
            if (abs(factor) > epsilon) {
               double *e=ppivot;
               for (int i=c+1; i<n; ++i) (*++e2)-=(*++e)*factor;
               B[r2]-=B[r]*factor;
            }
         }
      } else {
         throw degenerate_matrix();
      }
   }
   for (int c=n; c<m; ++c) {
      if (abs(B[row_index[c]]) > epsilon) throw infeasible();
   }

   Vector<double> x(n);
   for (int c=n-1; c>=0; --c) {
      x[c]=B[row_index[c]];
      for (int c2=0; c2<c; ++c2) {
         const int r2=row_index[c2];
         B[r2] -= x[c] * A(r2,c);
      }
   }

   return x;
}

} // end namespace pm

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
