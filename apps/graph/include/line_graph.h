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

#pragma once

#include "polymake/Graph.h"

namespace polymake { namespace graph {

namespace {

template <typename in_edge_iterator, typename out_edge_iterator>
bool stop_at(const in_edge_iterator&, const out_edge_iterator& out_edge_it, Directed)
{
   return out_edge_it.at_end();
}

template <typename edge_iterator>
bool stop_at(const edge_iterator& edge1_it, const edge_iterator& edge2_it, Undirected)
{
   return edge1_it == edge2_it;
}

}

template <typename Kind>
Graph<typename Kind::non_multi_type> line_graph(const Graph<Kind>& G)
{
   G.enumerate_edges();
   Graph<typename Kind::non_multi_type> result(G.edges());

   for (auto node_it = nodes(G).begin(); !node_it.at_end();  ++node_it) {
      for (auto in_edge_it = G.in_edges(*node_it).begin(); !in_edge_it.at_end();  ++in_edge_it) {
	 for (auto out_edge_it = G.out_edges(*node_it).begin();
	      !stop_at(in_edge_it, out_edge_it, typename Kind::non_multi_type());
	      ++out_edge_it) {
	    result.edge(*in_edge_it, *out_edge_it);
	 }
      }
   }

   return result;
}

} }


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
