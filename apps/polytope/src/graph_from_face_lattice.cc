/* Copyright (c) 1997-2021
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
#include "polymake/PowerSet.h"
#include "polymake/graph/Lattice.h"
#include "polymake/graph/Decoration.h"
#include "polymake/graph/lattice_migration.h"

namespace polymake { namespace polytope {

template <typename Decoration, typename SeqType>
Graph<> vertex_graph(BigObject HD_obj)
{
   const graph::Lattice<Decoration, SeqType> HD(HD_obj);
   const Int hd_rank = HD.rank();
   if (hd_rank<=0) return Graph<>(0);
   Graph<> G(HD.nodes_of_rank(1).size());

   // vertex sets stored by the polytope edge faces (dim==1)
   // are exactly the vertex pairs we need for the graph
   if (hd_rank > 1) {
      for (auto f_it = entire(attach_member_accessor(select(HD.decoration(), HD.nodes_of_rank(2)), ptr2type<graph::lattice::BasicDecoration, Set<Int>, &graph::lattice::BasicDecoration::face>())); !f_it.at_end(); ++f_it) {
         G.edge(f_it->front(), f_it->back());
      }
   }
   return G;
}

template <typename Decoration, typename SeqType>
Graph<> facet_graph(BigObject HD_obj)
{
   const graph::Lattice<Decoration, SeqType> HD(HD_obj);
   const Int hd_rank = HD.rank();
   if (hd_rank<=0) return Graph<>(0);
   const auto& facet_nodes = HD.nodes_of_rank(hd_rank-1);
   Graph<> G(facet_nodes.size());

   // the node numbers of the polytope facets (which are neighbors of the ridge faces, dim==-2)
   // relate to the whole Hasse diagram graph!
   Int node_shift = facet_nodes.front();
   if (hd_rank > 1) {
      for (auto f_it = entire(select(rows(adjacency_matrix(HD.graph())), HD.nodes_of_rank(hd_rank-2)));
            !f_it.at_end(); ++f_it) {
         if (f_it->size() > 1)
            for (auto pair_it = entire(all_subsets_of_k(*f_it,2)); !pair_it.at_end(); ++pair_it)
               G.edge( pair_it->front() - node_shift, pair_it->back() - node_shift);
      }
   }
   return G;
}

FunctionTemplate4perl("vertex_graph<Decoration,SeqType>(Lattice<Decoration, SeqType>)");
FunctionTemplate4perl("facet_graph<Decoration,SeqType>(Lattice<Decoration, SeqType>)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
