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

#pragma once

#include "polymake/client.h"
#include "polymake/list"
#include "polymake/FaceMap.h"
#include "polymake/graph/Decoration.h"
#include "polymake/graph/Lattice.h"
#include "polymake/graph/BasicLatticeTypes.h"

namespace polymake { namespace graph { namespace lattice_builder {

template <typename LType, bool dual>
void add_edge(LType& lattice, Int from, Int to, bool_constant<dual>)
{
  if (dual)
    lattice.add_edge(to, from);
  else
    lattice.add_edge(from, to);
}

using Primal = std::false_type;
using Dual = std::true_type;

/*
 * This runs the algorithm by Ganter / Kaibel,Pfetsch for a general closure system to compute the
 * lattice of all closed sets.
 * See also the paper "Algorithms for Tight Spans and Tropical Linear Spaces" 
 * (Simon Hampe, Michael Joswig, Benjamin Schröter), preprint (2016), arXiv: 1612.03592
 * @param ClosureOperator cl. This computes the actual closures. see graph/include/Closure.h for 
 * more information. 
 * @param CrossCut cut. This computes a cross cut in the lattice of closed systems. More precisely,
 * it has a boolean operator (..), which decides whether a set should be kept. The list of included nodes
 * is separated from the non-included nodes by a cross cut. See graph/include/BasicLatticeTypes.h
 * @param Decorator decorator. Knows how to compute the decoration (i.e. rank, face,..) of a node.
 * See graph/include/BasicLatticeTypes.h
 * @param bool wants_artificial_top_node Whether an additional node should be added on top at the end.
 * The decoration of this node is constructed by a special method of the decorator
 * @param bool built_dually. If true, all edge directions are inverted during the algorithm.
 * @param Lattice<Decoration> lattice. Optional and empty by default. The algorithm is
 * initialized with this lattice. If it's not empty, ALL its nodes are by default queued to check
 * for closures. This can be changed with the next parameter.
 * @param std::list<Int> queueing_nodes. Optional and empty by default. If the initial lattice is
 * not empty, this is the list of nodes that should be queued to check for closures. If it is empty,
 * all nodes are queued.
 */
template <typename Decoration, typename ClosureOperator, typename CrossCut, typename Decorator, bool dual, typename SeqType = lattice::Nonsequential>
Lattice<Decoration, SeqType> compute_lattice_from_closure(
            ClosureOperator cl,
            const CrossCut& cut,
            const Decorator& decorator,
            bool wants_artificial_top_node,
            bool_constant<dual> built_dually,
            Lattice<Decoration, SeqType> lattice = Lattice<Decoration>(),
            Set<Int> queuing_nodes = Set<Int>())
{
  using FaceData = typename ClosureOperator::ClosureData;
  std::list<std::pair<FaceData, Int>> Q; // queue of faces, which have been seen but who's faces above have not been computed yet.

  const NodeMap<Directed, Decoration>& decor = lattice.decoration();
  // If we start with a trivial lattice, we initialize the queue with closure(empty set)
  // Otherwise we initialize the queue with the nodes below the top node
  if (lattice.graph().nodes() == 0) {
    const FaceData first_node = cl.closure_of_empty_set();
    Decoration first_data = decorator.compute_initial_decoration(first_node);
    Int first_index = lattice.add_node(first_data);
    Q.push_back(std::make_pair(first_node, first_index) );
    cl.get_indexing_data(first_node).set_index(first_index);
  } else {
    sequence all_nodes = sequence(0, lattice.graph().nodes());
    if(queuing_nodes.size() == 0) {
      queuing_nodes = all_nodes;
    }
    for(auto i : all_nodes) {
      FaceData n_data = cl.compute_closure_data(decor[i]);
      cl.get_indexing_data(n_data).set_index(i);
      if (queuing_nodes.contains(i))
        Q.push_back(std::make_pair(n_data,i));
    }
  }

  std::list<Int> max_faces;

  while (__builtin_expect(!Q.empty(),1)) {
    std::pair<FaceData, Int> H = Q.front(); Q.pop_front();
    const Decoration H_data = decor[H.second];
    bool is_max_face = true;
    for (auto faces = cl.get_closure_iterator(H.first); !faces.at_end(); ++faces) {
      lattice::FaceIndexingData node_index_data = cl.get_indexing_data(*faces);
      if (node_index_data.is_unknown) {
        Decoration node_data = decorator.compute_decoration(*faces, H_data);
        if (cut(node_data)) {
          Int node_index = lattice.add_node(node_data);
          node_index_data.set_index(node_index);
          Q.push_back(std::make_pair(*faces, node_index));
        } else {
          node_index_data.mark_face_as_unwanted(); continue;
        }
      } else if (node_index_data.is_marked_unwanted) { // We already saw it and decided not to take it
        continue;
      }
      add_edge(lattice, H.second, node_index_data.index, built_dually);
      is_max_face = false;
    }
    if (is_max_face) max_faces.push_back(H.second);
  }

  if (wants_artificial_top_node) {
    Decoration max_decoration = decorator.compute_artificial_decoration(decor, max_faces);
    Int final_index = lattice.add_node(max_decoration);
    for (auto mf : max_faces) {
      add_edge(lattice, mf, final_index, built_dually);
    }
  }

  return lattice;
}

} } }


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
