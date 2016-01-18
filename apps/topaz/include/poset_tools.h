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

namespace {

template<typename Poset>
void complete_map(const Poset& P,
                  const Poset& Q,
                  const typename Entire<Edges<Poset> >::const_iterator& peit,
                  Array<int> current_map, // intentionally pass a copy
                  Set<Array<int> >& homs)
{
   typedef typename Entire<Edges<Poset> >::const_iterator EdgesIterator;

   for (EdgesIterator e=peit; !e.at_end(); ++e) {
      if (current_map[e.from_node()] < 0 &&
          current_map[e.to_node()] < 0) { // both nodes of the edge of P are still unassigned under current_map
         for (EdgesIterator qit = entire(edges(Q)); !qit.at_end(); ++qit) {
            // assign the edge of P to all edges of Q in turn and recurse
            current_map[e.from_node()]  = qit.from_node();
            current_map[e.to_node()] = qit.to_node();
            EdgesIterator next_e(e); ++next_e;
            complete_map(P, Q, next_e, current_map, homs);
         }
      } else if (current_map[e.from_node()] < 0) { // the from node is still unassigned
         for (typename Entire<typename Poset::in_adjacent_node_list>::const_iterator iit = entire(Q.in_adjacent_nodes(current_map[e.to_node()])); !iit.at_end(); ++iit) {
            // assign the edge of P to all edges of Q that enter e.to_node() in turn and recurse
            current_map[e.from_node()] = *iit;
            EdgesIterator next_e(e); ++next_e;
            complete_map(P, Q, next_e, current_map, homs);
         }
      } else if (current_map[e.to_node()] < 0) { // the to node is still unassigned
         for (typename Entire<typename Poset::out_adjacent_node_list>::const_iterator oit = entire(Q.out_adjacent_nodes(current_map[e.from_node()])); !oit.at_end(); ++oit) {
            // assign the edge of P to all edges of Q that leave e.from_node() in turn and recurse
            current_map[e.to_node()] = *oit;
            EdgesIterator next_e(e); ++next_e;
            complete_map(P, Q, next_e, current_map, homs);
         }
      } else if (!Q.edge_exists(current_map[e.from_node()], current_map[e.to_node()])) { // the induced edge is invalid
         return;
      }
   }
   // all edges of P are now assigned
   homs += current_map;
}

template<typename Poset>
void map_isolated_vertices(const Poset& P,
                           const Poset& Q,
                           const Array<int>& prescribed_map,
                           Set<Array<int> >& homs)
{
   Set<int> isolated_vertices(sequence(0, P.nodes()));

   // first remove vertices incident to edges
   for(typename Entire<Edges<Poset> >::const_iterator eit = entire(edges(P)); !eit.at_end(); ++eit) {
      isolated_vertices -= eit.from_node();
      isolated_vertices -= eit.to_node();
   }

   // then classify isolated vertices according to whether their image is prescribed or not
   Set<int> 
          prescribed_isolated_vertices,
      not_prescribed_isolated_vertices;
   for (Entire<Set<int> >::const_iterator iit = entire(isolated_vertices); !iit.at_end(); ++iit) {
      if (prescribed_map[*iit] == -1) 
         not_prescribed_isolated_vertices += *iit;
      else
             prescribed_isolated_vertices += *iit;
   }

   for (Entire<Set<int> >::const_iterator vit = entire(not_prescribed_isolated_vertices); !vit.at_end(); ++vit) {
      // the image of *vit will be -1 in all homomorphisms, so first replace that -1 with the first node of Q throughout
      Set<Array<int> > tmp_homs;
      for (Entire<Set<Array<int> > >::const_iterator hit = entire(homs); !hit.at_end(); ++hit) {
         Array<int> hom(*hit);
         hom[*vit] = 0;
         for (Entire<Set<int> >::const_iterator pvit = entire(prescribed_isolated_vertices); !pvit.at_end(); ++pvit)
            hom[*pvit] = prescribed_map[*pvit];
         tmp_homs += hom;
      }
      homs.swap(tmp_homs); // do it like this so that the Set tree doesn't have to be rebuilt so often by removing the array with -1 and adding the one with 0 back in

      // now process the rest of the vertices
      for (int i=1; i<Q.nodes(); ++i) {
         for (Entire<Set<Array<int> > >::const_iterator hit = entire(tmp_homs); !hit.at_end(); ++hit) {
            Array<int> hom(*hit);
            hom[*vit] = i;
            homs += hom;
         }
      }
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


template<typename Poset>
Set<Array<int> > poset_homomorphisms_impl(const Poset& P, 
                                          const Poset& _Q,
                                          Array<int> prescribed_map = Array<int>())
{
   Poset Q(_Q);
 
   // include loops in Q, to allow for contracting edges of P
   for (int i=0; i<Q.nodes(); ++i)
      Q.edge(i,i);

   if (!prescribed_map.size())
      prescribed_map = Array<int>(P.nodes(), -1);
   else if (prescribed_map.size() != P.nodes())
      throw std::runtime_error("The size of the given prescribed map does not match that of the domain poset");

   Set<Array<int> > homs;
   complete_map(P, Q, entire(edges(P)), prescribed_map, homs);

   map_isolated_vertices(P, Q, prescribed_map, homs);

   return homs;
}


template<typename Poset>
Poset hom_poset_impl(const Set<Array<int> >& homs, const Poset& Q)
{
   Poset H(homs.size());
   int i(0), j(0);
   for (Entire<Set<Array<int> > >::const_iterator hit1 = entire(homs); !hit1.at_end(); ++hit1, ++i) {
      Entire<Set<Array<int> > >::const_iterator hit2 = hit1;
      for (++hit2, j=i+1; !hit2.at_end(); ++hit2, ++j) {
         if (f_less_or_equal_g(*hit1, *hit2, Q))
            H.edge(i,j);
         else if (f_less_or_equal_g(*hit2, *hit1, Q))
            H.edge(j,i);
      }
   }
   return H;
}

template<typename Poset>
Poset hom_poset_impl(const Poset& P, const Poset& Q)
{
   return hom_poset_impl(poset_homomorphisms_impl(P, Q), Q);
}

template<typename Poset>
Poset covering_relations_impl(const Poset& P)
{
   std::list<std::vector<int> > path_queue;
   Poset covers(P);
   for (int i=0; i<P.nodes(); ++i)
      if (!P.in_degree(i) && P.out_degree(i)) {
         std::vector<int> path;
         path.push_back(i);
         path_queue.push_back(path);
      }
   
   while (path_queue.size()) {
      const std::vector<int> path(path_queue.front()); path_queue.pop_front();
      for (typename Entire<typename Poset::out_adjacent_node_list>::const_iterator oit = entire(P.out_adjacent_nodes(path.back())); !oit.at_end(); ++oit) {
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
