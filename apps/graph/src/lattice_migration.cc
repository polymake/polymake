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

#include "polymake/client.h"
#include "polymake/Graph.h"
#include "polymake/Array.h"
#include "polymake/graph/Decoration.h"
#include "polymake/graph/lattice_migration.h"

namespace polymake { namespace graph {

   using namespace lattice;

   template <typename SeqType>
   void migrate_hasse_properties(BigObject lattice) {
      const NodeMap<Directed, Set<Int>>& faces = lattice.give("FACES");
      const Array<Int>& dims = lattice.give("DIMS");
      const Graph<Directed>& g = lattice.give("ADJACENCY");

      bool built_dually = g.out_adjacent_nodes(0).size() == 0;
      Int rank = dims.size();
      Int top_node = built_dually? 0 : g.nodes()-1;
      Int bottom_node = built_dually? g.nodes()-1 : 0;

      InverseRankMap<SeqType> irm;
      std::list<BasicDecoration> decor;

      auto faces_it = entire(faces);
      for(auto dims_it = dim_to_rank_iterator<SeqType>(rank, g.nodes(), built_dually, dims); !dims_it.at_end(); ++dims_it) {
         if(!SeqType::trivial(dims_it->second)) irm.set_rank_list(dims_it->first, dims_it->second);
         for(auto list_it = entire(SeqType::map_value_as_container(dims_it->second)); !list_it.at_end(); ++list_it, ++faces_it) {
            decor.push_back(BasicDecoration(*faces_it, dims_it->first));
         }
      }

      NodeMap<Directed, BasicDecoration> decor_map(g, entire(decor));
      lattice.take("DECORATION") << decor_map;
      lattice.take("INVERSE_RANK_MAP") << irm;
      lattice.take("TOP_NODE") << top_node;
      lattice.take("BOTTOM_NODE") << bottom_node;
   }

   FunctionTemplate4perl("migrate_hasse_properties<SeqType>(Lattice<BasicDecoration, SeqType>)");

   FunctionTemplate4perl("faces_map_from_decoration(props::Graph, NodeMap)");

}}
