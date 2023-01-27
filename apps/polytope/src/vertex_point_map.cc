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
#include "polymake/Array.h"

namespace polymake { namespace polytope {

template <typename Matrix1, typename Matrix2, typename Scalar>
Array<Int> vertex_point_map(const GenericMatrix<Matrix1, Scalar>& verts, const GenericMatrix<Matrix2, Scalar>& points)
{
  const Int n = verts.rows();
  Array<Int> vpm(n, -1);

  for (Int i = 0; i < n; ++i) {
    for (Int j = 0; j < points.rows(); ++j) 
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
