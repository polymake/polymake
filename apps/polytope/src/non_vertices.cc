/* Copyright (c) 1997-2023
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
#include "polymake/Set.h"

namespace polymake { namespace polytope {

template <typename Matrix1, typename Matrix2, typename Scalar>
Set<Int> non_vertices(const GenericMatrix<Matrix1, Scalar>& points, const GenericMatrix<Matrix2, Scalar>& verts)
{
  const Int n_verts = verts.rows();
  const Int n_points = points.rows();

  Set<Int> non_vertices;

  if (3*n_verts<n_points) {
    non_vertices = sequence(0, n_points);
    for (Int i = 0; i < n_verts; ++i) {
      for (Int j = 0; j < n_points; ++j) 
        if (verts.row(i) == points.row(j)) {
          non_vertices.erase(j);
          break;
        }
    }
  } else {
    for (Int i = 0; i < n_points; ++i) {
      bool found = false;
      for (Int j = 0; j < n_verts; ++j) 
         if (verts.row(j) == points.row(i)) {
            found = true;
            break;
         }
      if (!found) non_vertices.push_back(i); 
    }
  }
  return non_vertices;
}

FunctionTemplate4perl("non_vertices(Matrix Matrix)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
