/* Copyright (c) 1997-2015
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
#include "polymake/Rational.h"
#include "polymake/Vector.h"
#include "polymake/Matrix.h"
#include "polymake/Set.h"
#include "polymake/list"
#include <string>

namespace polymake { namespace tropical {
namespace {
inline
Vector<Rational> normalized(const Vector<Rational>& v)
{
   const Rational m=accumulate(v, operations::min());
   return v-same_element_vector(m,v.dim());
}
}

Matrix<Rational> discard_non_vertices(const Matrix<Rational>& points)
{
   Matrix<Rational> V=points;
   const int n(V.rows()), d(V.cols());
   Set< Vector<Rational> > vertex_coords;
   Set<int> vertex_indices;
 
   for (int i=0; i<n; ++i) {
      const Rational c(V(i,0));
      V[i]-=same_element_vector(c,d);
      if (vertex_coords.contains(V[i])) continue; // notice that it is possible that a point arises more than once
      Set<int> sectors;
      for (int j=0; sectors.size()<d && j<n; ++j) {
         if (V[j]==V[i]) continue;
         const Vector<Rational> diff(normalized(V[j]-V[i]));
         for (int k=0; k<d; ++k)
            if (diff[k]==0) sectors+=k;
      }
      if (sectors.size()<d) {
         vertex_coords+=V[i];
         vertex_indices+=i;
      }
   }
   return V.minor(vertex_indices,sequence(0,d));
}

UserFunction4perl("# @category Producing a tropical polytope"
                  "# Given points in the tropical projective space, discard all the non-vertices of the tropical convex hull."
                  "# @param Matrix points"
		  		  "# @return Matrix",
                  &discard_non_vertices, "discard_non_vertices");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
