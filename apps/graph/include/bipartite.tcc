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

#include "polymake/graph/graph_iterators.h"
#include "polymake/graph/bipartite.h"
#include "polymake/graph/connected.h"
#include <cstdlib>

namespace polymake { namespace graph {

class BipartiteColoring {
protected:
   std::vector<int> color;
   int sign;
public:
   static const bool visit_all_edges=true;

   BipartiteColoring();

   template <typename TGraph>
   explicit BipartiteColoring(const GenericGraph<TGraph>& G)
      : color(G.top().dim(), 0)
      , sign(0)
   {}

   template <typename TGraph>
   void clear(const GenericGraph<TGraph>& G)
   {
      std::fill(color.begin(), color.end(), 0);
      sign=0;
   }

   bool operator()(int n)
   {
      color[n]=1;
      sign=1;
      return true;
   }

   bool operator()(int n_from, int n_to)
   {
      if (color[n_to]==0) {
         sign += color[n_to]=-color[n_from];
         return true;
      } else if (color[n_to]==color[n_from]) {
         throw n_to;
      } else {
         return false;
      }
   }

   int get_sign() const { return std::abs(sign); }
   int get_color(int n) const { return color[n]; }
};

template <typename TGraph>
int bipartite_sign(const GenericGraph<TGraph,Undirected>& G)
{
   int signature=0;
   for (connected_components_iterator<TGraph> C(G);  !C.at_end();  ++C) {
      int this_node=C->front();
      BFSiterator<TGraph, VisitorTag<BipartiteColoring> > it(G, this_node);
      try {
         while (!it.at_end()) ++it;
      } catch (int) {
         return -1;
      }
      signature += it.node_visitor().get_sign();
   }
   return signature;
}

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
