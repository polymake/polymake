/* Copyright (c) 1997-2014
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
#include "polymake/Array.h"
#include "polymake/graph/HasseDiagram.h"

namespace polymake { namespace polytope {

void edge_colored_bounded_graph(perl::Object g, const graph::HasseDiagram& HD, const Set<int>& far_face, const int upper_bound)
{
   const int n=HD.nodes();
   Array<int> highest_rank_seen(n,-1);

   for (int k= upper_bound>=0? upper_bound : HD.dim()-1; k>=2; --k)
      for (Entire<sequence>::iterator it=entire(HD.node_range_of_dim(k)); !it.at_end(); ++it) {
         const int this_idx=*it;
         const Set<int>& this_face = HD.face(this_idx);
         if ((this_face*far_face).empty()) {
            int rank=k;
            for (Graph<Directed>::out_edge_list::const_iterator e=HD.out_edges(this_idx).begin(); !e.at_end(); ++e)
               rank=std::max(rank,highest_rank_seen[e.to_node()]);
            highest_rank_seen[this_idx]=rank;
         }
      }

   const Graph<> BG=g.give("ADJACENCY");
   EdgeMap<Undirected, int> edge_colors(BG);

   for (Entire<sequence>::iterator it=entire(HD.node_range_of_dim(1)); !it.at_end(); ++it) {
      const int this_idx=*it;
      const Set<int>& this_edge = HD.face(this_idx);
      if ((this_edge*far_face).empty()) {
         int rank=1;
         for (Graph<Directed>::out_edge_list::const_iterator e=HD.out_edges(this_idx).begin(); !e.at_end(); ++e)
            assign_max(rank,highest_rank_seen[e.to_node()]);
         edge_colors(this_edge.front(), this_edge.back())=rank;
      }
   }

   g.take("EDGE_COLORS") << edge_colors;
}

Function4perl(&edge_colored_bounded_graph, "edge_colored_bounded_graph(Graph FaceLattice $; $=-1)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
