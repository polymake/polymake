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
#include "polymake/Map.h"
#include "polymake/graph/HasseDiagram.h"

namespace polymake { namespace polytope {
namespace {

typedef Map<int,int> face_map_type;

template <typename IM> inline
face_map_type count(const GenericIncidenceMatrix<IM>& adj, const sequence& face_indices)
{
   face_map_type face_map;

   for (Entire<sequence>::const_iterator f=entire(face_indices); !f.at_end(); ++f)
      face_map[adj.col(*f).size()]++;
   return face_map;
}
}

face_map_type two_face_sizes(perl::Object p)
{
   const graph::HasseDiagram HD(p);
   return count(adjacency_matrix(HD.graph()), HD.node_range_of_dim(2));
}

face_map_type subridge_sizes(perl::Object p)
{
   const graph::HasseDiagram HD(p);
   return count(T(adjacency_matrix(HD.graph())), HD.node_range_of_dim(-3));
}

Function4perl(&two_face_sizes, "two_face_sizes(FaceLattice)");
Function4perl(&subridge_sizes, "subridge_sizes(FaceLattice)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
