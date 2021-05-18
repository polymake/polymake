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

#include "polymake/client.h"
#include "polymake/Matrix.h"
#include "polymake/ListMatrix.h"
#include "polymake/Set.h"
#include "polymake/linalg.h"

namespace polymake { namespace polytope {

template <typename Scalar>
ListMatrix<Vector<Scalar> > jarvis(const Matrix<Scalar> &Points)
{
   if (Points.cols() != 3)
      throw std::runtime_error("jarvis: polytope is not 2-dimensional");

   const Int n = Points.rows();
   // the jarvis code needs at least 3 points to work properly:
   //   avoid invalid iterator derefencing
   //   doesnt detect duplicate points for n=2
   if (n < 3) {
      if (n == 2 && Points[0] == Points[1])
         return Points.minor(scalar2set(0), All);
      return Points;
   }

   Set<Int> points_left = range(0, n-1);
   ListMatrix<Vector<Scalar>> CH(0, 3);

   // find the lowest point, among those of the same level take the leftmost one
   Int i = 0;
   Scalar x(Points(0, 1));
   Scalar y(Points(0, 2));
   for (Int j = 1; j < n; ++j) {
      const Scalar& xx = Points(j, 1);
      const Scalar& yy = Points(j, 2);
      const Int s = sign(y-yy);
      if (s > 0 || (s == 0 && xx < x)) {
         i = j;
         x = xx;
         y = yy;
      }
   }
   CH /= Points.row(i); // that's a point on the convex hull

   // now perform "gift wrapping" starting at i
   const Int start = i;
   Matrix<Scalar> M(3,3);
   for (;;) {
      auto it = points_left.begin(), stop = points_left.end();
      if (*it == i) ++it; // can occur only in the first round
      Int current = *it; ++it;
      M[0] = Points[i];
      M[1] = Points[current];
      while (it != stop) {
         M[2] = Points[*it];
         const Int sd = sign(det(M));
         if (sd < 0 || (sd == 0 && sqr(M[2]-M[0]) > sqr(M[1]-M[0]))) {
            current = *it;
            M[1] = Points[current];
         }
         ++it;
      }
      
      if (current == start) break;

      CH /= Points.row(current);
      points_left -= current; // a point on the convex hull
      i = current;
   }

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
