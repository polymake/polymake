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

#include "polymake/topaz/complex_tools.h"

namespace polymake { namespace topaz {

bool is_pure(const Lattice<BasicDecoration>& HD)
{
   Int test_dim = -1;
   for (auto it=entire(HD.in_edges(HD.top_node())); !it.at_end(); ++it) {
      const Int n = it.from_node();

      if (test_dim == -1)  // first facet
         test_dim = HD.face(n).size()-1;
      else if ( HD.face(n).size()-1 != test_dim )
         return false;
   }
   return true;
}

Set<Int> vertices_of_vertex_link(const Lattice<BasicDecoration>& HD, const Int v)
{
   Set<Int> V;
   accumulate_in(vertex_star_in_HD(HD,v), operations::add(), V);
   V -= v;
   return V;
}

void remove_vertex_star(ShrinkingLattice<BasicDecoration>& HD, const Int v)
{
   graph::BFSiterator< Graph<Directed> > n_it(HD.graph(), find_vertex_node(HD,v));
   const Int top_node = HD.top_node();

   // remove all nodes of the HD that can be reached from start_node
   while (!n_it.at_end()) {
      const Int n = *n_it;  ++n_it;
      if (n != top_node) {
         for (auto e=entire(HD.in_edges(n)); !e.at_end(); ++e) {
            const Int nn = e.from_node();
            if (HD.out_degree(nn) == 1)
               HD.graph().edge(nn,top_node);
         }
         HD.graph().out_edges(n).clear();
         HD.graph().in_edges(n).clear();
      }
   }

   HD.delete_nodes(n_it.node_visitor().get_visited_nodes()-top_node);
   HD.set_implicit_top_rank();
}

void remove_facet_node(ShrinkingLattice<BasicDecoration>& HD, const Int start_node)
{
   graph::BFSiterator<Graph<Directed>, graph::TraversalDirectionTag<int_constant<-1>>> n_it(HD.graph(), start_node);
   const Int bottom_node = HD.bottom_node();
   HD.graph().out_edges(start_node).clear();
   Set<Int> to_delete;

   // remove all nodes of the HD that can be reached from start_node only
   while (!n_it.at_end()) {
      const Int n = *n_it;
      if (n == bottom_node || HD.graph().out_degree(n)) {
         n_it.skip_node();
      } else {
         to_delete+=n;
         ++n_it;
         HD.graph().in_edges(n).clear();
      }
   }

   HD.delete_nodes(to_delete);
   HD.set_implicit_top_rank();
}

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
