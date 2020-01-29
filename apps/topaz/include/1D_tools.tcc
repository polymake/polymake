/* Copyright (c) 1997-2020
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

#include "polymake/graph/connected.h"
#include "polymake/Graph.h"

namespace polymake { namespace topaz {
namespace {

// return values: 1=true, 0=false, -1=undef (does not occur here)
template <typename Complex>
Int fill_graph(Graph<>& G, const Complex& C, Int* bad_link_p)
{
   // check whether each vertex is contained in 1 or 2 edges
   for (auto c_it = entire(C); !c_it.at_end(); ++c_it) {
      auto f_it = c_it->begin();
      const Int n1 = *f_it;
      const Int n2 = *++f_it;
      G.edge(n1, n2);
      if (G.degree(n1) > 2) {
         if (bad_link_p) *bad_link_p = n1;
         return 0;
      }
      if (G.degree(n2) > 2) {
         if (bad_link_p) *bad_link_p = n2;
         return 0;
      }
   }

   return 1;
}

}

// return values: 1=true, 0=false, -1=undef (does not occur here)
template <typename Complex, typename VertexSet>
Int is_ball_or_sphere(const Complex& C, const GenericSet<VertexSet>& V, int_constant<1>)
{
   Graph<> G(V);
   // check graph for three properties
   // (1) connected
   // (2) degree(v)<3 all v in V
   // (3) #{v|degree(v)=1} =: n_leafs equals 0 or 2
   if (fill_graph(G, C, nullptr) == 0 || !graph::is_connected(G)) return 0;

   Int n_leaves = 0;
   for (auto v = entire(V.top()); !v.at_end(); ++v)
      if (G.degree(*v) == 1) {
         if (++n_leaves > 2) return 0;
      }

   return n_leaves != 1 ? 1 : 0;
}

// return values: 1=true, 0=false, -1=undef (does not occur here)
template <typename Complex, typename VertexSet>
Int is_manifold(const Complex& C, const GenericSet<VertexSet>& V, int_constant<1>, Int* bad_link_p)
{
   Graph<> G(V);
   return fill_graph(G, C, bad_link_p);
}

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
