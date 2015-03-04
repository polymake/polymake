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

#ifndef __POLYMAKE_MATROID_MODULAR_CUT_H__
#define __POLYMAKE_MATROID_MODULAR_CUT_H__

#include "polymake/Array.h"
#include "polymake/Set.h"
#include "polymake/PowerSet.h"
#include "polymake/Map.h"
#include "polymake/graph/HasseDiagram.h"
#include <list>

namespace polymake { namespace matroid {


/*
  A modular cut is a subset C of a lattice of flats such that
   (i) C is an up-set, i.e., if  F in C  and  F subset G , then  G in C
  (ii) If  F,G in C  with  r(F) + r(G) = r(F union G) + r(F intersection G) , 
       where r() is the rank function, then  F intersection G in C.
       The pair (F,G) is then called a modular pair of flats.
 */
bool is_modular_cut_impl(const Array<Set<int> >& C, const graph::HasseDiagram& LF)
{
   // prepare data structures for lattice of flats
   Map<Set<int>, int> rank_of, index_of;
   rank_of[Set<int>()] = -1;
   for (int i=0; i<=LF.dim(); ++i) {
      for (Entire<graph::HasseDiagram::nodes_of_dim_set>::const_iterator fit = entire(LF.nodes_of_dim(i)); !fit.at_end(); ++fit) {
         rank_of[LF.face(*fit)] = i;
         index_of[LF.face(*fit)] = *fit;
      }
   }
   
   // prepare data structures for modular cut
   Set<int> C_index_set;
   std::list<int> queue;
   Set<int> queue_set;
   for (Entire<Array<Set<int> > >::const_iterator cit = entire(C); !cit.at_end(); ++cit) {
      const int i(index_of[*cit]);
      C_index_set += i;
      queue.push_back(i);
      queue_set += i;
   }
   
   // Is C an up-set?
   while (queue_set.size()) {
      const int c = queue.front(); queue.pop_front(); queue_set -= c;
      for (Graph<Directed>::out_adjacent_node_list::const_iterator containing_flats_it = LF.out_adjacent_nodes(c).begin(); !containing_flats_it.at_end(); ++containing_flats_it) {
         if (!C_index_set.contains(*containing_flats_it)) {
            cout << "The given set is not an up-set. It contains the flat " << LF.face(c) 
                 << " but not the flat " << LF.face(*containing_flats_it) << endl;
            return false;
         }
      }
   }

   // Is C closed under intersections of modular pairs of flats?
   for (Entire<Subsets_of_k<const Set<int>&> >::const_iterator p=entire(all_subsets_of_k(C_index_set, 2)); !p.at_end(); ++p) {
      const Set<int> 
         pair(*p),
         F(LF.face(pair.front())),
         G(LF.face(pair.back())),
         FintG(F*G),
         FcupG(F+G);
      if (rank_of[F] + rank_of[G] == rank_of[FintG] + rank_of[FcupG] &&
          !C_index_set.contains(index_of[FintG])) {
         cout << "The given set is not closed under intersection of modular pairs of flats.\n"
              << "It contains the sets " << F << " of rank " << rank_of[F]
              << " and " << G << " of rank " << rank_of[G]
              << ", but not the intersection " << FintG << endl;
         return false;
      }
   }
   return true;
}

} }

#endif // __POLYMAKE_MATROID_MODULAR_CUT_H__

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
