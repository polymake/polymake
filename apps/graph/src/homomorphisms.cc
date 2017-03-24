/* Copyright (c) 1997-2017
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
#include "polymake/Graph.h"
#include "polymake/topaz/poset_tools.h"

namespace polymake { namespace graph {

Array<Array<int>>
graph_homomorphisms(const perl::Object g, const perl::Object h, perl::OptionSet options)
{
   const Graph<Undirected> G = g.give("ADJACENCY");

   // now make H into a directed graph, so that each edge of G will be mapped to both orientations of each edge of H
   const Graph<Directed>  _H = h.give("ADJACENCY");
   Graph<Directed> H(_H);
   for (auto eh = entire(edges(_H)); !eh.at_end(); ++eh)
      H.edge(eh.to_node(), eh.from_node());

   const Array<int> prescribed_map = options["prescribed_map"];
   const bool allow_loops = options["allow_loops"];
   topaz::RecordKeeper<topaz::HomList> record_keeper;
   
   const auto result(topaz::poset_homomorphisms_impl(G, H, record_keeper, prescribed_map, allow_loops));
   return Array<Array<int>>(result.size(), entire(result));
}

int
n_graph_homomorphisms(const perl::Object g, const perl::Object h, perl::OptionSet options)
{
   const Graph<Undirected> G = g.give("ADJACENCY");

   // now make H into a directed graph, so that each edge of G will be mapped to both orientations of each edge of H
   const Graph<Directed>  _H = h.give("ADJACENCY");
   Graph<Directed> H(_H);
   for (auto eh = entire(edges(_H)); !eh.at_end(); ++eh)
      H.edge(eh.to_node(), eh.from_node());

   const Array<int> prescribed_map = options["prescribed_map"];
   const bool allow_loops = options["allow_loops"];
   topaz::RecordKeeper<int> record_keeper;
   
   return topaz::poset_homomorphisms_impl(G, H, record_keeper, prescribed_map, allow_loops);
}
      
UserFunction4perl("# @category Combinatorics\n"
                  "# Enumerate all homomorphisms (edge-preserving maps) from one graph to another"
                  "# @param Graph G"
                  "# @param Graph H"
                  "# @option Bool allow_loops Should edges of G be allowed to collapse to a loop when mapped to H? Default 0"
                  "# @option Array<Int> prescribed_map A vector of length G.nodes() with those images in G that should be fixed. Negative entries will be enumerated over."
                  "# @return Array<Array<Int>>",
                  &graph_homomorphisms,
                  "graph_homomorphisms(Graph, Graph { allow_loops => 0, prescribed_map => []  })"); 

UserFunction4perl("# @category Combinatorics\n"
                  "# Count all homomorphisms (edge-preserving maps) from one graph to another."
                  "# They are in fact enumerated, but only the count is kept track of using constant memory."
                  "# @param Graph G"
                  "# @param Graph H"
                  "# @option Bool allow_loops Should edges of G be allowed to collapse to a loop when mapped to H? Default 0"
                  "# @option Array<Int> prescribed_map A vector of length G.nodes() with those images in G that should be fixed. Negative entries will be enumerated over."
                  "# @return Int",
                  &n_graph_homomorphisms,
                  "n_graph_homomorphisms(Graph, Graph { allow_loops => 0, prescribed_map => []  })"); 
      
} }


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
