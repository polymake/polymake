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
#include "polymake/graph/Lattice.h"
#include "polymake/graph/Decoration.h"

namespace polymake { namespace graph { namespace lattice {

template <>
void InverseRankMap<Nonsequential>::set_rank(Int n, Int r)
{
  inverse_rank_map[r].push_back(n);
}

template <>
void InverseRankMap<Sequential>::set_rank(Int n, Int r)
{
  auto p = inverse_rank_map.find(r);
  if (p.at_end()) {
    inverse_rank_map[r] = std::make_pair(n,n);
  } else {
    p->second.first= std::min(p->second.first,n);
    p->second.second = std::max(p->second.second,n);
  }
}

template <>
void InverseRankMap<Nonsequential>::delete_node_and_squeeze(Int n, Int r)
{
  auto& rlist = inverse_rank_map[r];
  rlist.remove(n);
  if (rlist.empty()) inverse_rank_map.erase(r);
  for (auto& kv_pair : inverse_rank_map) {
    for (auto& n_index : kv_pair.second) {
      if(n_index > n) --n_index;
    }
  }
}

template <>
void InverseRankMap<Sequential>::delete_node_and_squeeze(Int n, Int r)
{
  for (auto& kv_pair : inverse_rank_map) {
    if (kv_pair.second.first > n) --kv_pair.second.first;
    if (kv_pair.second.second >= n) --kv_pair.second.second;
    if (kv_pair.second.second < kv_pair.second.first) inverse_rank_map.erase(r);
  }
}

template <>
Nonsequential::nodes_of_rank_ref_type InverseRankMap<Nonsequential>::nodes_of_rank(Int d) const
{
  auto p = inverse_rank_map.find(d);
  if (p.at_end()) {
    static const Nonsequential::nodes_of_rank_type empty_list{};
    return empty_list;
  }
  return p->second;
}

template <>
Sequential::nodes_of_rank_ref_type InverseRankMap<Sequential>::nodes_of_rank(Int d) const
{
  auto p = inverse_rank_map.find(d);
  if (p.at_end()) {
    static const Sequential::nodes_of_rank_type empty_list{};
    return empty_list;
  }
  return Sequential::map_value_as_container(p->second);
}

template <>
Nonsequential::nodes_of_rank_type InverseRankMap<Nonsequential>::nodes_of_rank_range(Int d1, Int d2) const
{
  Nonsequential::nodes_of_rank_type result;
  if (d2 < d1) std::swap(d2, d1);
  auto mpval = inverse_rank_map.find_nearest(d1, operations::ge());
  while (!mpval.at_end() && mpval->first <= d2) {
    result.insert(result.end(), mpval->second.begin(), mpval->second.end());
    ++mpval;
  }
  return result;
}

template <>
Sequential::nodes_of_rank_type InverseRankMap<Sequential>::nodes_of_rank_range(Int d1, Int d2) const
{
  if (d2 < d1) std::swap(d2, d1);
  auto lower_seq = inverse_rank_map.find_nearest(d1, operations::ge());
  auto upper_seq = inverse_rank_map.find_nearest(d2, operations::le());
  if (!lower_seq.at_end() && !upper_seq.at_end()) {
    Int lower_bound = std::min(lower_seq->second.first, upper_seq->second.first);
    Int upper_bound = std::max(lower_seq->second.second, upper_seq->second.second);
    return Sequential::nodes_of_rank_type(lower_bound, upper_bound-lower_bound+1);
  }
  return Sequential::nodes_of_rank_type();
}

} } }
