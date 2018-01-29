/* Copyright (c) 1997-2018
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

#ifndef POLYMAKE_GRAPH_LATTICE_PERMUTATION_H
#define POLYMAKE_GRAPH_LATTICE_PERMUTATION_H

#include "polymake/client.h"
#include "polymake/Graph.h"
#include "polymake/Set.h"
#include "polymake/Array.h"
#include "polymake/vector"
#include "polymake/IncidenceMatrix.h"
#include <algorithm>
#include "polymake/graph/Decoration.h"
#include "polymake/graph/Lattice.h"

namespace polymake { namespace graph { namespace lattice {

   template <typename Decoration>
   class CompareByFace {
      private:
         const NodeMap<Directed, Decoration>& node_map;
      public:
         CompareByFace(const NodeMap<Directed, Decoration>& nmap) : node_map(nmap) {}
         pm::cmp_value operator() (const int a, const int b) const {
            return operations::cmp()( node_map[a].face, node_map[b].face);
         }
   };

   //Sorts vertices by number and facets according to a specified order.
   //Facets are not sorted if VIF.rows == 0
   // @param Lattice The lattice to be sorted
   // @param IncidenceMatrix<> VIF A list of facets prescribing the order of the facet nodes. If
   // empty, only the rays are sorted.
   template <typename Decoration, typename SeqType>
      void sort_vertices_and_facets(Lattice<Decoration, SeqType> &l, const IncidenceMatrix<>& VIF) {
         Array<int> perm(sequence(0, l.nodes()));
         
         //First sort the vertices
         Set<int> unsorted_nodes( l.nodes_of_rank(1));
         Set<int, CompareByFace<Decoration> > vertex_nodes(CompareByFace<Decoration>(l.decoration()));
         for(auto nd : unsorted_nodes) { vertex_nodes += nd;}
         copy_range( entire(vertex_nodes), select(perm, unsorted_nodes).begin());

         //Then facets
         if(l.rank() > 2 && VIF.rows() > 0) {
            Array<int> facet_nodes(l.nodes_of_rank(l.rank()-1));
            const IncidenceMatrix<> lfacets(VIF.rows(), VIF.cols(), 
                  entire(attach_member_accessor( select(l.decoration(), facet_nodes), 
                     ptr2type< Decoration, Set<int>, &Decoration::face>())));
            Array<int> fperm = find_permutation(rows(lfacets), rows(VIF));
            copy_range( entire(permuted(facet_nodes, fperm)), select(perm, facet_nodes).begin()); 
         }

         l.permute_nodes_in_levels(perm);
      }

   // This can be used to make a Lattice sequential. 
   // Careful! This is fairly expensive for large lattices, since all data needs to be copied.
	template <typename Decoration>
		perl::Object make_lattice_sequential(const Lattice<Decoration, lattice::Nonsequential> &lattice) {
			Array<int> node_perm(lattice.nodes());
			InverseRankMap<lattice::Sequential> new_rank_map;
			int current_index = 0;
			for(auto rk_it = entire(lattice.inverse_rank_map().get_map()); !rk_it.at_end(); ++rk_it) {
				int list_length = rk_it->second.size();
				copy_range(entire(sequence(current_index, list_length)), select(node_perm, rk_it->second).begin());
				std::pair<int,int> new_map_value(current_index, current_index + list_length - 1);
				new_rank_map.set_rank_list( rk_it->first, new_map_value);
				current_index += list_length;
			}

			Graph<Directed> new_graph = permuted_inv_nodes( lattice.graph(), node_perm);
			NodeMap<Directed, Decoration> new_decoration(new_graph);
			copy_range( entire(lattice.decoration()), select(new_decoration, node_perm).begin());

			perl::Object result(
					perl::ObjectType::construct<typename pm::concat_list<Decoration, lattice::Sequential>::type>(
						"Lattice"));
			result.take("ADJACENCY") << new_graph;
			result.take("DECORATION") << new_decoration;
			result.take("INVERSE_RANK_MAP") << new_rank_map;
			result.take("TOP_NODE") << node_perm[ lattice.top_node()];
			result.take("BOTTOM_NODE") << node_perm[ lattice.bottom_node()];
			return result;
		}


}}}

#endif
