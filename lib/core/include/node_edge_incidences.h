/* Copyright (c) 1997-2023
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

#include "polymake/SparseMatrix.h"
#include "polymake/Graph.h"

namespace pm {

// the node-edge incidence matrix of a directed graph is totally unimodular
template <typename Coord, typename Graph> inline
SparseMatrix<Coord> node_edge_incidences(const GenericGraph<Graph, graph::Directed>& G)
{
   SparseMatrix<Coord> ne(G.top().nodes(), G.top().edges());
   Int k = 0;
   for (auto e = entire(edges(G)); !e.at_end(); ++e, ++k) {
      ne(e.to_node(),   k) = 1;
      ne(e.from_node(), k) =-1;
   }
   return ne;
}

// this is the undirected case
// the node-edge incidence matrix of a /bipartite/ graph is also totally unimodular
template <typename Coord, typename Graph> inline
SparseMatrix<Coord> node_edge_incidences(const GenericGraph<Graph, graph::Undirected>& G)
{
   SparseMatrix<Coord> ne(G.top().nodes(), G.top().edges());
   Int k = 0;
   for (auto e=entire(edges(G)); !e.at_end(); ++e, ++k) {
      ne(e.to_node(),   k) = 1;
      ne(e.from_node(), k) = 1;
   }
   return ne;
}

}
namespace polymake {
using pm::node_edge_incidences;
}


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
