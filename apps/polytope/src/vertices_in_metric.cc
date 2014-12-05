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
#include "polymake/Array.h"
#include "polymake/Map.h"
#include "polymake/Matrix.h"

namespace polymake { namespace polytope {

template<typename Scalar>
Array<int> vertices_in_metric(const Matrix<Scalar> &verts, const Matrix<Scalar>& metric)
{

  Map<Vector<Scalar>,int> elems;
  const int n=metric.rows();
  const int n_verts=verts.rows();
  for (int i=0; i<n;++i)
    elems[metric.row(i)]=i;

  Array<int> vim(n_verts,-1);

  for (int i=0; i<n_verts; ++i)
    if (verts(i,0) == 1) {
      const typename Map<Vector<Scalar>, int>::const_iterator value = elems.find(verts.row(i).slice(1));
      if (value != elems.end())
        vim[i] = value->second;
    }
    
  return vim;
}

FunctionTemplate4perl("vertices_in_metric<Scalar>(Matrix<type_upgrade<Scalar>> Matrix<type_upgrade<Scalar>>)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
