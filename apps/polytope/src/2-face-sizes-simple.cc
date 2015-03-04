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

/** @file 2-face-sizes-simple
 *
 *  Compute the sizes of all 2-faces of a simple polytope.
 *  Since this algorithm does not require the complete face lattice, it is much faster
 *  than the standard client @see 2-face-sizes.
 *
 */

#include "polymake/client.h"
#include "polymake/Map.h"
#include "polymake/Graph.h"
#include "polymake/IncidenceMatrix.h"

namespace polymake { namespace polytope {
namespace {

typedef Map<int,int> face_map_type;

template <typename IMatrix>
face_map_type count(const Graph<>& G, const GenericIncidenceMatrix<IMatrix>& I)
{
   face_map_type face_map;

   // Each adjacent pair of edges spans a 2-face.  In order to avoid double counting we enumerate adjacent pairs of
   // edges such that the middle vertex is maximal among all vertices of that 2-face.
   // The to_nodes in the out_edge_list come in ascending order.
   for (int n1=G.nodes()-1; n1>=0; --n1)
      for (Entire<Graph<>::out_edge_list>::const_iterator n2=G.out_edges(n1).begin(); !n2.at_end() && n2.to_node()<n1; ++n2) {
	 for (Entire<Graph<>::out_edge_list>::const_iterator n3=G.out_edges(n1).begin(); n3.to_node()<n2.to_node(); ++n3) {
	    const Set<int> facets_thru_all3(I.col(n1)*I.col(n2.to_node())*I.col(n3.to_node()));

	    // compute the 2-face spanned by n1, n2.to_node() and n3.to_node()
	    const Set<int> this_face=accumulate(rows(I.minor(facets_thru_all3,All)), operations::mul());

	    if (this_face.back()==n1)        // n1 largest vertex in 2-face; this avoids double counting
	       ++face_map[this_face.size()];
	 }
      }

   return face_map;
}
} // end anonymous namespace

void two_face_sizes_simple(perl::Object p)
{
   const Graph<> G=p.give("GRAPH.ADJACENCY");
   const IncidenceMatrix<> I=p.give("VERTICES_IN_FACETS");
   p.take("TWO_FACE_SIZES") << count(G,I);
}

void subridge_sizes_simple(perl::Object p)
{
   const Graph<> G=p.give("DUAL_GRAPH.ADJACENCY");
   const IncidenceMatrix<> I=p.give("VERTICES_IN_FACETS");
   p.take("SUBRIDGE_SIZES") << count(G,T(I));
}

Function4perl(&two_face_sizes_simple, "two_face_sizes_simple(Polytope)");
Function4perl(&subridge_sizes_simple, "subridge_sizes_simple(Polytope)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
