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
#include "polymake/Map.h"
#include "polymake/graph/Lattice.h"
#include "polymake/graph/Decoration.h"

namespace polymake { namespace polytope {
namespace {

using face_map_type = Map<Int, Int>;

template <typename IM, typename NodeList>
face_map_type count(const GenericIncidenceMatrix<IM>& adj, const NodeList& face_indices)
{
   face_map_type face_map;

   for (const auto f : face_indices)
      ++face_map[adj.col(f).size()];
   return face_map;
}
}

face_map_type two_face_sizes(BigObject p)
{
   const graph::Lattice<graph::lattice::BasicDecoration, graph::lattice::Sequential> HD(p);
   return count(adjacency_matrix(HD.graph()), HD.nodes_of_rank(3));
}

face_map_type subridge_sizes(BigObject p)
{
   const graph::Lattice<graph::lattice::BasicDecoration, graph::lattice::Sequential> HD(p);
   Int top_rank = HD.rank();
   return count(T(adjacency_matrix(HD.graph())), HD.nodes_of_rank(top_rank-3));
}

Function4perl(&two_face_sizes, "two_face_sizes(Lattice<BasicDecoration, Sequential>)");
Function4perl(&subridge_sizes, "subridge_sizes(Lattice<BasicDecoration, Sequential>)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
