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

#include "polymake/client.h"
#include "polymake/Graph.h"
#include "polymake/Array.h"
#include "polymake/graph/Decoration.h"
#include "polymake/tropical/covectors.h"
#include "polymake/graph/lattice_migration.h"

namespace polymake { namespace tropical {

   using namespace graph;
   using namespace graph::lattice;

   void migrate_hasse_properties(perl::Object lattice) {
      const NodeMap<Directed, Set<int> >& faces = lattice.give("FACES");
      const NodeMap<Directed, IncidenceMatrix<> >& covectors = lattice.give("COVECTORS");
      const Array<int>& dims = lattice.give("DIMS");
      const Graph<Directed>& g = lattice.give("ADJACENCY");

      bool built_dually = g.out_adjacent_nodes(0).size() == 0;
      int rank = dims.size();
      int top_node = built_dually? 0 : g.nodes()-1;
      int bottom_node = built_dually? g.nodes()-1 : 0;

      InverseRankMap<Nonsequential> irm;
      std::list<CovectorDecoration> decor;

      auto faces_it = entire(faces);
      auto covectors_it = entire(covectors);
      for(auto dims_it = dim_to_rank_iterator<Nonsequential>(rank, g.nodes(), built_dually, dims); !dims_it.at_end(); ++dims_it) {
         if(!Nonsequential::trivial(dims_it->second)) irm.set_rank_list(dims_it->first, dims_it->second);
         for(auto list_it = entire(dims_it->second); !list_it.at_end(); ++list_it, ++faces_it, ++covectors_it) {
            decor.push_back( CovectorDecoration( *faces_it, dims_it->first, *covectors_it));
         }
      }

      NodeMap<Directed, CovectorDecoration> decor_map(g, entire(decor));
      lattice.take("DECORATION") << decor_map;
      lattice.take("INVERSE_RANK_MAP") << irm;
      lattice.take("TOP_NODE") << top_node;
      lattice.take("BOTTOM_NODE") << bottom_node;
   }

   NodeMap<Directed, IncidenceMatrix<> > covector_map_from_decoration(const Graph<Directed>& graph, const NodeMap<Directed, CovectorDecoration>& decor) {
      return NodeMap<Directed, IncidenceMatrix<> >(
               graph,
               entire(attach_member_accessor(decor, ptr2type<CovectorDecoration, IncidenceMatrix<>, &CovectorDecoration::covector>()))
            );
   }

   Function4perl(&migrate_hasse_properties, "migrate_hasse_properties(CovectorLattice)");
   Function4perl(&covector_map_from_decoration, "covector_map_from_decoration(props::Graph, NodeMap)");

}}
