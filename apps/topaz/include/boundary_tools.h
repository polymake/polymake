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

#pragma once

namespace polymake { namespace topaz {

//the following takes non-redundant, non-including faces and adjusts the numbering,
//returning a map from new indices to old ones. used in connection with boundary constructions.
struct ind2map_consumer {
  mutable Array<Int> map;
  mutable Int n_verts = 0;
  ind2map_consumer(Int n)
    : map(n) {};

  void operator() (const Int& old_index, Int& new_index) const
  {
    map[new_index] = old_index;
    n_verts = std::max(n_verts, new_index+1);
  }

  Array<Int> give_map()
  {
    return Array<Int>(n_verts, map.begin());
  }
};

inline      
std::pair<Array<Set<Int>>, Array<Int>>
squeeze_faces(IncidenceMatrix<> faces)
{
  auto c = ind2map_consumer(faces.cols());
  faces.squeeze_cols(c);
  return std::make_pair(Array<Set<Int>>(rows(faces)), c.give_map());
}

} }


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
