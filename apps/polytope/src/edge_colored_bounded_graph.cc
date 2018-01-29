/* Copyright (c) 1997-2018
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
#include "polymake/graph/Lattice.h"
#include "polymake/graph/Decoration.h"

namespace polymake { namespace polytope {

using namespace graph;
using namespace graph::lattice;

void edge_colored_bounded_graph(
		const Array<int>& max_poly_comb_dims,
		const IncidenceMatrix<>& max_polys,
		perl::Object g)
{
   const Graph<> BG=g.give("ADJACENCY");
	const Array<Set<int> > edges = g.call_method("EDGES");
   EdgeMap<Undirected, int> edge_colors(BG);

	for(auto e = ensure(edges, (pm::cons<pm::indexed, pm::end_sensitive>*)0).begin(); !e.at_end(); ++e) {
		int rank = 1;
		auto mc_dim_it = entire(max_poly_comb_dims);
		auto mc_it = entire(rows(max_polys));
		for(;!mc_it.at_end(); ++mc_it, ++mc_dim_it) {
			if(incl(*e,*mc_it) <= 0)
				assign_max(rank, *mc_dim_it);
		}
		edge_colors[e.index()] = rank;
	}

   g.take("EDGE_COLORS") << edge_colors;
}

Function4perl(&edge_colored_bounded_graph, "edge_colored_bounded_graph(Array<Int>, IncidenceMatrix, Graph<Undirected>)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
