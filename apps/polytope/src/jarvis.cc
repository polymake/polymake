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

#include "polymake/client.h"
#include "polymake/Matrix.h"
#include "polymake/ListMatrix.h"
#include "polymake/Set.h"
#include "polymake/linalg.h"

namespace polymake { namespace polytope {

template <typename Scalar>
ListMatrix<Vector<Scalar> > jarvis(const Matrix<Scalar> &Points)
{
   if (Points.cols()!=3)
      throw std::runtime_error("jarvis: polytope is not 2-dimensional");

   const int n=Points.rows();
   // the jarvis code needs at least 3 points to work properly:
   //   avoid invalid iterator derefencing
   //   doesnt detect duplicate points for n=2
   if (n < 3) {
      if (n == 2 && Points[0] == Points[1])
         return Points.minor(scalar2set(0),All);
      return Points;
   }

   Set<int> points_left=range(0,n-1);
   ListMatrix< Vector<Scalar > > CH(0,3);

   // find the lowest point, among those of the same level take the leftmost one
   int i=0;
   Scalar x(Points(0,1));
   Scalar y(Points(0,2));
   for (int j=1; j<n; ++j) {
      const Scalar& xx=Points(j,1);
      const Scalar& yy=Points(j,2);
      switch (sign(y-yy)) {
      case 1: i=j; x=xx; y=yy; break;
      case 0: if (xx<x) {i=j; x=xx; y=yy;} break;
      default: break;
      }
   }
   CH/=Points.row(i); // that's a point on the convex hull

   // now perform "gift wrapping" starting at i
   const int start=i;
   Matrix<Scalar> M(3,3);
   bool done=false;
   do {
      Set<int>::iterator it=points_left.begin(), stop=points_left.end();
      if (*it==i) ++it; // can occur only in the first round
      int current=*it; ++it;
      M[0]=Points[i];
      M[1]=Points[current];
      while (it!=stop) {
         M[2]=Points[*it];
         switch (sign(det(M))) {
         case -1: current=*it; M[1]=Points[current]; break;
         case 0:
            if (sqr(M[2]-M[0]) > sqr(M[1]-M[0])) {
               current=*it; M[1]=Points[current];
            }
            break;
         default: break;
         }
         ++it;
      }
      
      if (current==start) {
         done=true;
      } else {
         CH/=Points.row(current);
         points_left-=current; // a point on the convex hull
         i=current;
      }
   } while (!done);

   return CH;
}

// Compute the convex hull of a 2-dimensional point set via Jarvis' March.
// Complexity: O(n log(n))
// The points returned are in cyclic order.
FunctionTemplate4perl("jarvis(Matrix)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
