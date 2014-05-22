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

#include "polymake/client.h"
#include "polymake/Matrix.h"
#include "polymake/Set.h"

namespace polymake { namespace polytope {

template <typename Scalar>
Set<int> non_vertices(const Matrix<Scalar>& points, const Matrix<Scalar>& verts)
{
  const int n_verts=verts.rows();
  const int n_points=points.rows();

  Set<int> non_vertices;

  if (3*n_verts<n_points) {
    non_vertices=sequence(0,n_points);
    for (int i=0; i<n_verts; ++i) {
      for (int j=0; j<n_points; ++j) 
        if (verts.row(i)==points.row(j)) {
          non_vertices.erase(j);
          break;
        }
    }
  }
  else {
    for (int i=0; i<n_points; ++i) {
      bool found=false;
      for (int j=0; j<n_verts; ++j) 
        if (verts.row(j)==points.row(i)) {
          found=true;
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
