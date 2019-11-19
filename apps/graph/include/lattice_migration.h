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

#ifndef POLYMAKE_GRAPH_LATTICE_MIGRATION_H
#define POLYMAKE_GRAPH_LATTICE_MIGRATION_H

#include "polymake/Graph.h"
#include "polymake/Array.h"
#include "polymake/graph/Decoration.h"

namespace polymake { namespace graph {

/*
 * This iterator is used for converting the old DIMS array of HasseDiagram/FaceLattice to the new
 * InverseRankMap. It iterates over pairs (rank, list of node of this rank).
 */
template <typename SeqType>
class dim_to_rank_iterator {
public:
  using iterator_category = std::forward_iterator_tag;
  using value_type = std::pair<int, typename SeqType::map_value_type>;
  using reference = const value_type&;
  using pointer = const value_type*;
  using difference_type = ptrdiff_t;

  dim_to_rank_iterator(int total_rank_, int total_size_, bool built_dually_, const Array<int>& dims_)
    : total_rank(total_rank_)
    , total_size(total_size_)
    , built_dually(built_dually_)
    , dims(dims_)
    , current_dims_index(0)
    , current_index_bound(0)
  {
    if (!dims.empty()) current_index_bound = dims[0];
    result = std::make_pair(built_dually ? total_rank : 0,
                            SeqType::make_map_value_type(0, std::max(current_index_bound, 1)-1));
  }

  reference operator* () const { return result; }
  pointer operator->() const { return &result; }

  dim_to_rank_iterator& operator++ () { find_next(); return *this; }
  const dim_to_rank_iterator operator++ (int) { dim_to_rank_iterator copy = *this; operator++(); return copy; }

  bool at_end() const { return current_dims_index > dims.size(); }

protected:
  void find_next()
  {
    ++current_dims_index;
    if (!at_end()) {
      int old_index_bound = current_index_bound;
      current_index_bound = current_dims_index == dims.size()? total_size : dims[current_dims_index];
      int next_rank = result.first + (built_dually? -1 : 1);
      result = std::make_pair(next_rank,
                              SeqType::make_map_value_type(old_index_bound, current_index_bound-1));
    }
  }

  const int total_rank;
  const int total_size;
  const bool built_dually;
  const Array<int>& dims;
  int current_dims_index;
  int current_index_bound;
  value_type result;
};

/*
 * @brief Computes a NodeMap which only contains the face of nodes.
 */
template <typename Decoration>
NodeMap<Directed, Set<int>>
faces_map_from_decoration(const Graph<Directed>& graph, const NodeMap<Directed, Decoration>& decor)
{
  return NodeMap<Directed, Set<int> >(
           graph,
           entire(attach_member_accessor(decor, ptr2type<Decoration,Set<int>, &Decoration::face>()))
           );
}

} }

#endif
