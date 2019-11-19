/* Copyright (c) 1997-2019
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

#include "polymake/graph/DijkstraShortestPath.h"
#include "polymake/graph/DijkstraShortestPathWithScalarWeights.h"
#include "polymake/Array.h"
#include "polymake/vector"

namespace polymake {
namespace graph {

//! find the shortest path in a graph between two given nodes
//! for the given edge weights
//! @return List(Array<int>, Weight>) path as a sequence of nodes and total weight
//!                                   empty if there is no path connecting source and target nodes
template <typename Dir, typename Weight>
perl::ListReturn shortest_path_dijkstra(const Graph<Dir>& G, const EdgeMap<Dir, Weight>& weights,
                                        int source_node, int target_node, bool backward)
{
   if (source_node<0 || source_node>=G.nodes())
      throw std::runtime_error("invalid source node");
   if (target_node<0 || target_node>=G.nodes())
      throw std::runtime_error("invalid source node");

   perl::ListReturn result;
   DijkstraShortestPath<DijkstraShortestPathWithScalarWeights<Dir, Weight>> DSP(G, weights);
   auto path_it = DSP.solve(source_node, target_node, backward);
   if (!path_it.at_end()) {
      const Weight w = path_it.cur_weight();
      std::vector<int> rev_path;
      do
         rev_path.push_back(path_it.cur_node());
      while (!(++path_it).at_end());
      result << Array<int>(rev_path.size(), rev_path.rbegin());
      result << w;
   }

   return result;
}

UserFunctionTemplate4perl("# Find the shortest path in a graph"
                          "# @param Graph G a graph without parallel edges"
                          "# @param EdgeMap weights edge weights"
                          "# @param Int source the source node"
                          "# @param Int target the target node"
                          "# @param Bool if true, perform backward search",
                          "shortest_path_dijkstra(props::Graph, EdgeMap, $, $; $=0)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
