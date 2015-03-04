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

#include "polymake/graph/BFSiterator.h"
#include "polymake/graph/bipartite.h"
#include "polymake/graph/connected.h"
#include <cstdlib>

namespace polymake { namespace graph {

class BipartiteColoring {
protected:
   std::vector<int> color;
   int sign;
public:
   BipartiteColoring();

   template <typename Graph>
   BipartiteColoring(const GenericGraph<Graph>& G, int start_node)
      : color(G.top().dim(),0), sign(1)
   {
      if (!color.empty()) color[start_node]=1; else sign=0;
   }

   template <typename Graph>
   void reset(const GenericGraph<Graph>&, int start_node)
   {
      fill(entire(color),0);
      if (!color.empty()) color[start_node]=1, sign=1; else sign=0;
   }

   bool seen(int n) const { return color[n]!=0; }

   void add(int n, int n_from)
   {
      sign += color[n]=-color[n_from];
   }

   static const bool check_edges=true;
   void check(int n, int n_from)
   {
      if (color[n]==color[n_from]) throw n;
   }

   int get_sign() const { return std::abs(sign); }
   int get_color(int n) const { return color[n]; }
};

template <typename Graph>
int bipartite_sign(const GenericGraph<Graph,Undirected>& G)
{
   connected_components_iterator<Graph> C(G);
   int signature=0;
   while (!C.at_end()) {
      int this_node=C->front();
      BFSiterator<Graph, Visitor<BipartiteColoring> > it(G, this_node);
      try {
         while (!it.at_end()) ++it;
      } catch (int) {
         return -1;
      }
      signature += it.node_visitor().get_sign();
      ++C;
   }
   return signature;
}

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
