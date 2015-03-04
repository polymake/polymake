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
#include "polymake/Graph.h"
#include "polymake/Set.h"
#include "polymake/graph/BFSiterator.h"

namespace polymake { namespace graph {

NodeMap<Undirected,int> greedy_coloring(const Graph<>& G)
{
   NodeMap<Undirected,int> C(G,-1);

   BFSiterator< Graph<>, Visitor< BoolNodeVisitor<true> > > it(G, nodes(G).front());
   while (true) {
      while (!it.at_end()) {
         const int n=*it;
         Set<int> forbidden_colors;
         for (Entire<Graph<>::out_edge_list>::const_iterator e=entire(G.out_edges(n)); !e.at_end(); ++e)
            forbidden_colors += C[e.to_node()];
         forbidden_colors -= -1;
         C[n]=(range(0,forbidden_colors.size())-forbidden_colors).front();
         ++it;
      }
      if (it.unvisited_nodes()>0)
         it.reset(it.node_visitor().get_visited_nodes().front());
      else
         break;
   }

   return C;
}

Function4perl(&greedy_coloring, "greedy_coloring");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
