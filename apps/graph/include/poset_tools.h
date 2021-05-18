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

#pragma once

#include "polymake/Array.h"
#include "polymake/Set.h"
#include "polymake/Graph.h"
#include <list>

/** Tools to treat posets
 *
 *  A poset is represented as directed graph that contains _all_ comparison relations, not just the minimal ones.
 */


namespace polymake { namespace graph {

namespace poset_tools {

using Homomorphism = Array<Int> ;
using HomList = std::vector<Homomorphism>;
   
template <typename Record>
class RecordKeeper {
   Record record;
public:
   RecordKeeper() : record(Record()) {}
   void add(const Homomorphism& h) {}
   Record result() const { return record; }
   Record& result() { return record; }
};

template <> inline
void RecordKeeper<HomList>::add(const Homomorphism& h)
{
   record.push_back(h);
}

template <> inline
void RecordKeeper<Int>::add(const Homomorphism&)
{
   ++record;
}
      

using EdgeList = std::vector<std::pair<Int, Int>>;

enum class PEdgeStatus {
   not_fixed,
   compatible_with_map,
   incompatible_with_map
};   
   
template <typename Poset, typename Iterator>
PEdgeStatus compatibility_status(const Poset& Q, const Iterator& it, const Array<Int>& mapping) {
   if (mapping[it.from_node()] == -1 ||
       mapping[it.  to_node()] == -1) {
      return PEdgeStatus::not_fixed;
   } else {
      return Q.edge_exists(mapping[it.from_node()],
                           mapping[it.  to_node()])
         ? PEdgeStatus::compatible_with_map
         : PEdgeStatus::incompatible_with_map;
   }
}

// all edges of Q are relevant if both images of it.front(), it.back() are variable
// otherwise, an edge q of Q is relevant if exactly one of the images is variable, and q is incident to the non-variable end;
template< typename Poset, typename Iterator>
const EdgeList& relevant_q_edges(const Poset& Q,
                                 const Iterator& it,
                                 const Array<Int>& mapping,
                                 const EdgeList& Qedges,
                                 EdgeList& relevant_q_edges) 
{
   const Int mf = mapping[it.from_node()];
   const Int mt = mapping[it.  to_node()];
   if (mf == -1 && mt != -1) {
      for (auto qit = entire(Q.in_adjacent_nodes(mt)); !qit.at_end(); ++qit) {
         relevant_q_edges.push_back({ *qit, mt });
      }
   }
   if (mf != -1 && mt == -1) {
      for (auto qit = entire(Q.out_adjacent_nodes(mf)); !qit.at_end(); ++qit) {
         relevant_q_edges.push_back({ mf, *qit });
      }
   }
   return relevant_q_edges.size()
      ? relevant_q_edges
      : Qedges;
}

// check if the edge of P indicated by peit can be added to the current map, then recurse, produce output or return because of incompatibility   
template <typename PosetP, typename PosetQ, typename EdgesIterator, typename Record>
void complete_map(const PosetP& P,
                  const PosetQ& Q,
                  const EdgeList& Qedges,
                  const EdgesIterator& peit,
                  Int p_edges_placed, // edge count of edges in P; synchronized with peit 
                  Array<Int> current_map, // intentionally pass a copy
                  RecordKeeper<Record>& record_keeper)
{
   assert(p_edges_placed < P.edges()); // cannot handle P with no edges 
   const PEdgeStatus es(compatibility_status(Q, peit, current_map));
   if (es == PEdgeStatus::incompatible_with_map) {
      return;
   }
   if (es == PEdgeStatus::compatible_with_map) { // no modification of current_map necessary, placed compatible p-edge
      ++p_edges_placed;
      if (p_edges_placed == P.edges()) { // map is complete
         record_keeper.add(current_map);
         return;
      }
      // compatible edge, recursing further
      auto next_peit(peit); ++next_peit;
      complete_map(P, Q, Qedges, next_peit, p_edges_placed, current_map, record_keeper);
      return;
   }
   // now es == PEdgeStatus::not_fixed
   const Int pf = peit.from_node();
   const Int pt = peit.  to_node();
   const Int old_f = current_map[pf];
   const Int old_t = current_map[pt];

   assert(old_f == -1 || old_t == -1);
   EdgeList relevant_q_edge_list;
   for (auto qeit = entire(relevant_q_edges(Q, peit, current_map, Qedges, relevant_q_edge_list)); !qeit.at_end(); ++qeit) {
      // map *peit to *qeit
      assert(current_map[pf] == -1 || current_map[pf] == qeit->first);
      current_map[pf] = qeit->first;

      assert(current_map[pt] == -1 || current_map[pt] == qeit->second);
      current_map[pt] = qeit->second;

      auto next_peit(peit);
      ++p_edges_placed; ++next_peit;

      if (p_edges_placed == P.edges()) {
         record_keeper.add(current_map);
      } else {
         complete_map(P, Q, Qedges, next_peit, p_edges_placed, current_map, record_keeper);
      }
      --p_edges_placed;

      // reinstate map
      current_map[pf] = old_f;
      current_map[pt] = old_t;
   }
}

template<typename Poset>
void
classify_isolated_vertices(const Poset& P,
                           const Array<Int>& prescribed_map,
                           Set<Int>&     prescribed,
                           Set<Int>& not_prescribed)
{
   Set<Int> isolated_vertices(sequence(0, P.nodes()));

   // first remove vertices incident to edges
   for (auto eit = entire(edges(P)); !eit.at_end(); ++eit) {
      isolated_vertices -= eit.from_node();
      isolated_vertices -= eit.to_node();
   }

   // then classify isolated vertices according to whether their image is prescribed or not
   for (auto iit = entire(isolated_vertices); !iit.at_end(); ++iit) {
      if (prescribed_map[*iit] == -1) 
         not_prescribed += *iit;
      else
             prescribed += *iit;
   }
}

template<typename PosetP, typename PosetQ>
void map_isolated_vertices(const PosetP& P,
                           const PosetQ& Q,
                           const Array<Int>& prescribed_map,
                           RecordKeeper<HomList>& record_keeper)
{
   Set<Int> 
          prescribed_isolated_vertices,
      not_prescribed_isolated_vertices;
   classify_isolated_vertices(P, prescribed_map, prescribed_isolated_vertices, not_prescribed_isolated_vertices);

   if (record_keeper.result().size() == 0) {
      record_keeper.result().push_back(Array<Int>(P.nodes(), -1));
   }
   
   for (auto vit = entire(not_prescribed_isolated_vertices); !vit.at_end(); ++vit) {
      // the image of *vit will be -1 in all homomorphisms, so first replace that -1 with the first node of Q throughout
      HomList tmp_homs;
      for (auto hit = entire(record_keeper.result()); !hit.at_end(); ++hit) {
         Array<Int> hom(*hit);
         hom[*vit] = 0;
         for (auto pvit = entire(prescribed_isolated_vertices); !pvit.at_end(); ++pvit)
            hom[*pvit] = prescribed_map[*pvit];
         tmp_homs.push_back(hom);
      }
      record_keeper.result().swap(tmp_homs); // do it like this so that the Set tree doesn't have to be rebuilt so often by removing the array with -1 and adding the one with 0 back in

      // now process the rest of the vertices
      for (Int i = 1; i < Q.nodes(); ++i) {
         for (auto hit = entire(tmp_homs); !hit.at_end(); ++hit) {
            Array<Int> hom(*hit);
            hom[*vit] = i;
            record_keeper.result().push_back(hom);
         }
      }
   }
}

template<typename PosetP, typename PosetQ>
void map_isolated_vertices(const PosetP& P,
                           const PosetQ& Q,
                           const Array<Int>& prescribed_map,
                           RecordKeeper<Int>& record_keeper)
{
   Set<Int>
          prescribed_isolated_vertices,
      not_prescribed_isolated_vertices;
   classify_isolated_vertices(P, prescribed_map, prescribed_isolated_vertices, not_prescribed_isolated_vertices);

   // any not prescribed isolated vertex can go to any vertex in Q
   if (not_prescribed_isolated_vertices.size()) {
      if (record_keeper.result() == 0) { // no edges
         record_keeper.result() = 1;
      }
      record_keeper.result() *= not_prescribed_isolated_vertices.size() * Q.nodes();
   }
}


// compare two functions f,g: P --> Q by
// f <= g iff f(p) <= g(p) for all p in P.
template<typename Poset>
bool f_less_or_equal_g(const Array<Int>& f, const Array<Int>& g, const Poset& Q)
{
   assert(f.size() == g.size());
   for (Int i = 0; i < f.size(); ++i)
      if (f[i] != g[i] &&
          !Q.edge_exists(f[i], g[i]))
         return false;
   return true;
}

template<typename PosetP, typename PosetQ, typename Record>
auto poset_homomorphisms_impl(const PosetP& P,
                              const PosetQ& Q_,
                              RecordKeeper<Record>& record_keeper,
                              Array<Int> prescribed_map = Array<Int>(),
                              bool allow_loops = true)
{
   PosetQ Q(Q_);

   if (allow_loops) {
      // include loops in Q, to allow for contracting edges of P
      for (Int i = 0; i < Q.nodes(); ++i)
         Q.edge(i, i);
   }
   
   if (!prescribed_map.size())
      prescribed_map = Array<Int>(P.nodes(), -1);
   else if (prescribed_map.size() != P.nodes())
      throw std::runtime_error("The size of the given prescribed map does not match that of the domain poset");

   EdgeList Qedges;
   for (auto eit = entire(edges(Q)); !eit.at_end(); ++eit)
      Qedges.push_back({ eit.from_node(), eit.to_node() });

   if (P.edges()) {
      complete_map(P, Q, Qedges, entire(edges(P)), 0, prescribed_map, record_keeper);
   }
   map_isolated_vertices(P, Q, prescribed_map, record_keeper);

   return record_keeper.result();
}

template <typename PosetQ>
PosetQ hom_poset_impl(const HomList& homs, const PosetQ& Q)
{
   PosetQ H(homs.size());
   Int i = 0, j = 0;
   for (auto hit1 = entire(homs); !hit1.at_end(); ++hit1, ++i) {
      auto hit2 = hit1;
      for (++hit2, j=i+1; !hit2.at_end(); ++hit2, ++j) {
         if (f_less_or_equal_g(*hit1, *hit2, Q))
            H.edge(i, j);
         else if (f_less_or_equal_g(*hit2, *hit1, Q))
            H.edge(j, i);
      }
   }
   return H;
}

template<typename PosetQ>
PosetQ hom_poset_impl(const Array<Array<Int>>& homs, const PosetQ& Q)
{
   return hom_poset_impl(HomList(homs.begin(), homs.end()), Q);
}

template<typename PosetP, typename PosetQ>
auto hom_poset_impl(const PosetP& P, const PosetQ& Q)
{
   RecordKeeper<HomList> record_keeper;
   return hom_poset_impl(poset_homomorphisms_impl(P, Q, record_keeper), Q);
}

template<typename Poset>
Poset covering_relations_impl(const Poset& P)
{
   std::list<std::vector<Int>> path_queue;
   Poset covers(P);
   for (Int i = 0; i < P.nodes(); ++i)
      if (!P.in_degree(i) && P.out_degree(i)) {
         std::vector<Int> path;
         path.push_back(i);
         path_queue.push_back(path);
      }
   
   while (path_queue.size()) {
      const std::vector<Int> path(path_queue.front()); path_queue.pop_front();
      for (auto oit = entire(P.out_adjacent_nodes(path.back())); !oit.at_end(); ++oit) {
         for (Int j = 0; j < Int(path.size())-1; ++j)
            covers.delete_edge(path[j], *oit);
         if (P.out_degree(*oit))  {
            std::vector<Int> new_path(path);
            new_path.push_back(*oit);
            path_queue.push_back(new_path);
         }
      }
   }
   return covers;
}

} } }


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
