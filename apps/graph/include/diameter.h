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

#ifndef POLYMAKE_GRAPH_DIAMETER_H
#define POLYMAKE_GRAPH_DIAMETER_H

#include "polymake/GenericGraph.h"
#include "polymake/graph/graph_iterators.h"

namespace polymake { namespace graph {

class NodeDistances {
protected:
   std::vector<int> dist;
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

   bool operator()(int n)
   {
      dist[n]=0;
      return true;
   }

   bool operator()(int n_from, int n_to)
   {
      if (dist[n_to]<0) {
         dist[n_to]=dist[n_from]+1;
         return true;
      }
      return false;
   }

   int operator[] (int n) const { return dist[n]; }
};

/// Determine the diameter of a graph.
template <typename TGraph>
int diameter(const GenericGraph<TGraph>& G)
{
   BFSiterator<TGraph, VisitorTag<NodeDistances>> it(G);
   int diam=0;
   for (auto n=entire(nodes(G)); !n.at_end(); ++n) {
      for (it.reset(*n); it.undiscovered_nodes()>0; ++it) ;
      assign_max(diam, it.node_visitor()[it.get_queue().back()]);
   }
   return diam;
}

} }

#endif // POLYMAKE_GRAPH_DIAMETER_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
