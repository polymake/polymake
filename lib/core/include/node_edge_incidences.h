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

#ifndef POLYMAKE_NODE_EDGE_INCIDENCES_H
#define POLYMAKE_NODE_EDGE_INCIDENCES_H

#include "polymake/SparseMatrix.h"
#include "polymake/Graph.h"

namespace pm {

// the node-edge incidence matrix of a directed graph is totally unimodular
template <typename Coord, typename Graph> inline
SparseMatrix<Coord> node_edge_incidences(const GenericGraph<Graph, graph::Directed>& G)
{
   SparseMatrix<Coord> ne(G.top().nodes(), G.top().edges());
   int k=0;
   for (auto e=entire(edges(G)); !e.at_end(); ++e, ++k) {
      ne(e.to_node(),  k)=1;
      ne(e.from_node(),k)=-1;
   }
   return ne;
}

// this is the undirected case
// the node-edge incidence matrix of a /bipartite/ graph is also totally unimodular
template <typename Coord, typename Graph> inline
SparseMatrix<Coord> node_edge_incidences(const GenericGraph<Graph, graph::Undirected>& G)
{
   SparseMatrix<Coord> ne(G.top().nodes(), G.top().edges());
   int k=0;
   for (auto e=entire(edges(G)); !e.at_end(); ++e, ++k) {
      ne(e.to_node(),  k)=1;
      ne(e.from_node(),k)=1;
   }
   return ne;
}

}
namespace polymake {
using pm::node_edge_incidences;
}

#endif // POLYMAKE_NODE_EDGE_INCIDENCES_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
