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

#include "polymake/GenericGraph.h"
#include "polymake/graph/graph_iterators.h"

namespace polymake { namespace graph {

class NodeDistances {
protected:
   std::vector<Int> dist;
public:
   static const bool visit_all_edges=false;

   NodeDistances() {}

   template <typename TGraph>
   explicit NodeDistances(const GenericGraph<TGraph>& G)
      : dist(G.top().dim(), -1) {}

   template <typename TGraph>
   void clear(const GenericGraph<TGraph>& G)
   {
      std::fill(dist.begin(), dist.end(), -1);
   }

   bool operator()(Int n)
   {
      dist[n] = 0;
      return true;
   }

   bool operator()(Int n_from, Int n_to)
   {
      if (dist[n_to] < 0) {
         dist[n_to] = dist[n_from]+1;
         return true;
      }
      return false;
   }

   Int operator[] (Int n) const { return dist[n]; }
};

/// Determine the diameter of a graph.
template <typename TGraph>
Int diameter(const GenericGraph<TGraph>& G)
{
   BFSiterator<TGraph, VisitorTag<NodeDistances>> it(G);
   Int diam = 0;
   for (auto n = entire(nodes(G)); !n.at_end(); ++n) {
      for (it.reset(*n); it.undiscovered_nodes()>0; ++it) ;
      assign_max(diam, it.node_visitor()[it.get_queue().back()]);
   }
   return diam;
}

} }


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
