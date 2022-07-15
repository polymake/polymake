/* Copyright (c) 1997-2022
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
#include "polymake/Array.h"
#include "polymake/graph/Lattice.h"
#include "polymake/graph/Decoration.h"

namespace polymake { namespace polytope {

using namespace graph;
using namespace graph::lattice;

void edge_colored_bounded_graph(const Array<Int>& max_poly_comb_dims,
                                const IncidenceMatrix<>& max_polys,
                                BigObject g)
{
   const Graph<> BG=g.give("ADJACENCY");
   const Array<Set<Int>> edges = g.call_method("EDGES");
   EdgeMap<Undirected, Int> edge_colors(BG);

   for (auto e = entire<indexed>(edges); !e.at_end(); ++e) {
      Int rank = 1;
      auto mc_dim_it = entire(max_poly_comb_dims);
      auto mc_it = entire(rows(max_polys));
      for (;!mc_it.at_end(); ++mc_it, ++mc_dim_it) {
         if (incl(*e,*mc_it) <= 0)
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
