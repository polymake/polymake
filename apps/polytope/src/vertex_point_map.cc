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
#include "polymake/Array.h"

namespace polymake { namespace polytope {

template <typename Scalar>
Array<int> vertex_point_map(const Matrix<Scalar>& verts, const Matrix<Scalar>& points)
{
  const int n=verts.rows();
  Array<int> vpm(n,-1);

  for (int i=0; i<n; ++i) {
    for (int j=0; j<points.rows(); ++j) 
      if (verts.row(i)==points.row(j)) {
        vpm[i]=j;
        break;
      }
    if (vpm[i]==-1) throw std::runtime_error("vertex point mismatch");
  }
   return vpm;
}

FunctionTemplate4perl("vertex_point_map(Matrix Matrix)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
