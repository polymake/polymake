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

#ifndef POLYMAKE_TOPAZ_POSET_TOOLS_H
#define POLYMAKE_TOPAZ_POSET_TOOLS_H

#include "polymake/Array.h"
#include "polymake/Set.h"
#include "polymake/Graph.h"
#include <list>

/** Tools to treat posets
 *
 *  A poset is represented as directed graph that contains _all_ comparison relations, not just the minimal ones.
 */


namespace polymake { namespace topaz {

typedef Array<int> Homomorphism;
typedef std::vector<Homomorphism> HomList;
   
template<typename Record>
class RecordKeeper {
   Record record;
public:
   RecordKeeper() : record(Record()) {}
   void add(const Homomorphism& h) {}
   Record result() const { return record; }
   Record& result() { return record; }
};

template<>
void RecordKeeper<HomList>::add(const Homomorphism& h) {
   record.push_back(h);
}

template<>
void RecordKeeper<int>::add(const Homomorphism&) {
   ++record;
}
      
namespace {

typedef std::vector<std::pair<int,int>> EdgeList;

enum PEdgeStatus {
   not_fixed,
   compatible_with_map,
   incompatible_with_map
};   
   
template<typename Poset, typename Iterator>
PEdgeStatus compatibility_status(const Poset& Q, const Iterator& it, const Array<int>& mapping) {
   if (mapping[it.from_node()] == -1 ||
       mapping[it.  to_node()] == -1) {
      return not_fixed;
   } else {
      return Q.edge_exists(mapping[it.from_node()],
                           mapping[it.  to_node()])
         ? compatible_with_map
         : incompatible_with_map;
   }
}

// all edges of Q are relevant if both images of it.front(), it.back() are variable
// otherwise, an edge q of Q is relevant if exactly one of the images is variable, and q is incident to the non-variable end;
template<typename Poset, typename Iterator>
const EdgeList& relevant_q_edges(const Poset& Q,
                                 const Iterator& it,
                                 const Array<int>& mapping,
                                 const EdgeList& Qedges,
                                 EdgeList& relevant_q_edges) 
{
   const int 
      mf (mapping[it.from_node()]),
      mt (mapping[it.  to_node()]);
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
template<typename PosetP, typename PosetQ, typename Record>
void complete_map(const PosetP& P,
                  const PosetQ& Q,
                  const EdgeList& Qedges,
                  const typename Entire<Edges<PosetP>>::const_iterator peit,
                  int p_edges_placed, // edge count of edges in P; synchronized with peit 
                  Array<int> current_map, // intentionally pass a copy
                  RecordKeeper<Record>& record_keeper)
{
   assert(p_edges_placed < P.edges()); // cannot handle P with no edges 
   const PEdgeStatus es(compatibility_status(Q, peit, current_map));
   if (es == incompatible_with_map) {
      return;
   }
   if (es == compatible_with_map) { // no modification of current_map necessary, placed compatible p-edge
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
   // now es == not_fixed
   const int 
      pf(peit.from_node()),
      pt(peit.  to_node()),
      old_f(current_map[pf]),
      old_t(current_map[pt]);
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
                           const Array<int>& prescribed_map,
                           Set<int>&     prescribed,
                           Set<int>& not_prescribed)
{
   Set<int> isolated_vertices(sequence(0, P.nodes()));

   // first remove vertices incident to edges
   for(auto eit = entire(edges(P)); !eit.at_end(); ++eit) {
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
                           const Array<int>& prescribed_map,
                           RecordKeeper<HomList>& record_keeper)
{
   Set<int> 
          prescribed_isolated_vertices,
      not_prescribed_isolated_vertices;
   classify_isolated_vertices(P, prescribed_map, prescribed_isolated_vertices, not_prescribed_isolated_vertices);

   if (record_keeper.result().size() == 0) {
      record_keeper.result().push_back(Array<int>(P.nodes(), -1));
   }
   
   for (auto vit = entire(not_prescribed_isolated_vertices); !vit.at_end(); ++vit) {
      // the image of *vit will be -1 in all homomorphisms, so first replace that -1 with the first node of Q throughout
      HomList tmp_homs;
      for (auto hit = entire(record_keeper.result()); !hit.at_end(); ++hit) {
         Array<int> hom(*hit);
         hom[*vit] = 0;
         for (auto pvit = entire(prescribed_isolated_vertices); !pvit.at_end(); ++pvit)
            hom[*pvit] = prescribed_map[*pvit];
         tmp_homs.push_back(hom);
      }
      record_keeper.result().swap(tmp_homs); // do it like this so that the Set tree doesn't have to be rebuilt so often by removing the array with -1 and adding the one with 0 back in

      // now process the rest of the vertices
      for (int i=1; i<Q.nodes(); ++i) {
         for (auto hit = entire(tmp_homs); !hit.at_end(); ++hit) {
            Array<int> hom(*hit);
            hom[*vit] = i;
            record_keeper.result().push_back(hom);
         }
      }
   }
}

template<typename PosetP, typename PosetQ>
void map_isolated_vertices(const PosetP& P,
                           const PosetQ& Q,
                           const Array<int>& prescribed_map,
                           RecordKeeper<int>& record_keeper)
{
   Set<int> 
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
bool f_less_or_equal_g(const Array<int>& f, const Array<int>& g, const Poset& Q)
{
   assert(f.size() == g.size());
   for (int i=0; i<f.size(); ++i)
      if (f[i] != g[i] &&
          !Q.edge_exists(f[i], g[i]))
         return false;
   return true;
}

} // end anonymous namespace


template<typename PosetP, typename PosetQ, typename Record>
auto poset_homomorphisms_impl(const PosetP& P, 
                              const PosetQ& _Q,
                              RecordKeeper<Record>& record_keeper,
                              Array<int> prescribed_map = Array<int>(),
                              bool allow_loops = true)
{
   PosetQ Q(_Q);

   if (allow_loops) {
      // include loops in Q, to allow for contracting edges of P
      for (int i=0; i<Q.nodes(); ++i)
         Q.edge(i,i);
   }
   
   if (!prescribed_map.size())
      prescribed_map = Array<int>(P.nodes(), -1);
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


template<typename PosetQ>
PosetQ hom_poset_impl(const HomList& homs, const PosetQ& Q)
{
   PosetQ H(homs.size());
   int i(0), j(0);
   for (auto hit1 = entire(homs); !hit1.at_end(); ++hit1, ++i) {
      auto hit2 = hit1;
      for (++hit2, j=i+1; !hit2.at_end(); ++hit2, ++j) {
         if (f_less_or_equal_g(*hit1, *hit2, Q))
            H.edge(i,j);
         else if (f_less_or_equal_g(*hit2, *hit1, Q))
            H.edge(j,i);
      }
   }
   return H;
}

template<typename PosetQ>
PosetQ hom_poset_impl(const Array<Array<int>>& homs, const PosetQ& Q)
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
   std::list<std::vector<int>> path_queue;
   Poset covers(P);
   for (int i=0; i<P.nodes(); ++i)
      if (!P.in_degree(i) && P.out_degree(i)) {
         std::vector<int> path;
         path.push_back(i);
         path_queue.push_back(path);
      }
   
   while (path_queue.size()) {
      const std::vector<int> path(path_queue.front()); path_queue.pop_front();
      for (auto oit = entire(P.out_adjacent_nodes(path.back())); !oit.at_end(); ++oit) {
         for (size_t j=0; j<path.size()-1; ++j)
            covers.delete_edge(path[j], *oit);
         if (P.out_degree(*oit))  {
            std::vector<int> new_path(path);
            new_path.push_back(*oit);
            path_queue.push_back(new_path);
         }
      }
   }
   return covers;
}

} }

#endif // POLYMAKE_TOPAZ_POSET_TOOLS_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
