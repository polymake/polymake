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
#include "polymake/Bitset.h"
#include "polymake/Graph.h"

namespace polymake { namespace graph {
namespace {

typedef Graph<Directed> dgraph;

Int FF_rec(Int n, Int t, Bitset& visited, dgraph& G, EdgeMap<Directed, bool>& saturated)
{
   // DFS to find augmenting path, augmenting is done backwards once t is reached
   // return value is t if t is reached, n otherwise

   if (n==t)     // augmenting path is found
      return t;

   // traversing the outgoing edges (in the original graph)
   for (auto e = entire(G.out_edges(n)); !e.at_end(); ++e) {
      Int nn = e.to_node();
      if ( !visited.contains(nn) && !saturated[*e] ) {   // nn has not been visited and the arc (n,nn) is not saturated
         visited+=nn;
         Int return_node = FF_rec(nn, t, visited, G, saturated);
         if (return_node == t) {      // augmenting path is found: augment (original) edge
            saturated[*e] ^= 1;
            return t;
         }
      }
   }

   // traversing the ingoing edges (= reversed edges in the residual graph)
   for (auto e=entire(G.in_edges(n)); !e.at_end(); ++e) {
      Int nn = e.from_node();
      if ( !visited.contains(nn) && saturated[*e] ) {   // nn has not been visited and the arc (nn,n) is saturated, therefore it exists in the res graph
         visited+=nn;
         Int return_node = FF_rec(nn, t, visited, G, saturated);
         if (return_node == t) {      // augmenting path is found: augment (reverse) edge
            saturated[*e] ^= 1;
            return t;
         }
      }
   }

   return n;        // t has not been reached
}

Int FF(Int s, Int t, dgraph& G)
{
   Int maxflow = 0;
   EdgeMap<Directed, bool> saturated(G, false);

   for (;;) {
      Bitset visited(G.nodes());
      visited+=s;
      if (FF_rec(s, t, visited, G, saturated) != t) break;   // t wasn't reached -> no augmenting path found
      ++maxflow;
   }

   return maxflow;
}

}  // end unnamed namespace

template <typename AnyGraph>
Int connectivity(const GenericGraph<AnyGraph, Undirected>& G_in)
{
   /* Tranfsorm the graph:
    * each node n is split in n_in and n_out with an arc (n_in, n_out).
    * the arcs (x,n) are replaced by (x,n_in) and the arcs (n,y) by (n_out,y).
    * all edges are assumed to have capacity c=1.
    *
    * n_in will be represented as n and n_out as n+nodes
    *
    * if there is "something" flowing over an edge e, than e is immediately saturated,
    * since all edges have capacity c=1 and we are computing an integer flow. 
    */

   const Int nodes = G_in.nodes();
   dgraph G(2*nodes);
   for (Int i = 0; i < nodes; ++i) {
      G.out_adjacent_nodes(i+nodes) = G_in.top().adjacent_nodes(i);
      G.edge(i, i+nodes);
   }

   // compute min maxflow from node 0+nodes to node n for n=1,...,nodes-1
   // using Ford-Fulkerson
   Int minmaxflow = nodes;
   for (Int i = 1; i < nodes; ++i)
      assign_min(minmaxflow, FF(nodes,i,G));
     
   return minmaxflow;
}

UserFunctionTemplate4perl("# @category Combinatorics"
                          "# Compute the [[CONNECTIVITY]] of a given //graph// using the Ford-Fulkerson flow algorithm."
                          "# @param GraphAdjacency<Undirected> graph"
                          "# @return Int"
                          "# @example [application polytope]"
                          "# Compute the connectivity of the vertex-edge graph of the square:"
                          "# > print connectivity(cube(2)->GRAPH->ADJACENCY);"
                          "# | 2"
                          "# This means that at least two nodes or edges need to be removed in order"
                          "# for the resulting graph not to be connected anymore."
                          "# @author Nikolaus Witte",
                          "connectivity(GraphAdjacency<Undirected>)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
