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
#include "polymake/graph/Lattice.h"
#include "polymake/graph/Decoration.h"
#include "polymake/graph/LatticeTools.h"
#include <list>

namespace polymake { namespace matroid {

   using graph::Lattice;
   using graph::lattice::Sequential;
   using graph::lattice::BasicDecoration;
namespace {

template<typename HDType>
bool covering_condition(const Set<int>& Cset, const HDType& LF, const Map<Set<int>, int>& index_of, bool verbose) {
   for (typename Entire<Subsets_of_k<const Set<int>&> >::const_iterator pit=entire(all_subsets_of_k(Cset, 2)); !pit.at_end(); ++pit) {
      const Set<int> p(*pit);
      const int x(p.front()), y(p.back());
      const int join = index_of[LF.face(x) * LF.face(y)];
      /*
        Because of not(a => b) being equivalent to   a and (not b),
        not(x or y covers x^y => x^y in C)
        is equivalent to
        (x or y covers x^y), and x^y notin C
       */
      if (!Cset.contains(join) &&
          (LF.in_adjacent_nodes(x).contains(join) ||
           LF.in_adjacent_nodes(y).contains(join))) {
         if (verbose) cout << "The given set does not satisfy the covering condition because "
                           << "at least one of " << LF.face(x) << " and " << LF.face(y)
                           << " strictly covers their intersection " << LF.face(x) * LF.face(y)
                           << ", which is not in the cut"
                           << endl;
         return false;
      }
   }
   return true;
}

} // end anonymous namespace

/*
  A modular cut is a subset C of a lattice of flats such that
  (1) C is convex, i.e.  x,z in C, x<y<z  implies  y in C
  (2) C contains {0,...,n-1}
  (3) x,y in C,  x covers x^y  implies  x^y in C.
 */
template<typename SetType>
bool is_modular_cut_impl(const Array<SetType>& C, const Lattice<BasicDecoration, Sequential>& LF, bool verbose)
{
   // prepare data structures for lattice of flats
   Map<Set<int>, int> index_of;
   for (int i=0; i<=LF.rank(); ++i) {
      for (auto fit = entire(LF.nodes_of_rank(i)); !fit.at_end(); ++fit) {
         index_of[LF.face(*fit)] = *fit;
      }
   }

   Set<int> Cset;
   for (typename Entire<Array<SetType> >::const_iterator cit = entire(C); !cit.at_end(); ++cit) {
      Map<Set<int>, int>::const_iterator tmp = index_of.find(*cit);
      if(tmp == index_of.end()){
         if (verbose) cout << "The given array is not a modular cut because "
                           << *cit << " is  not a flat of the given matroid."
                           << endl;
         return false;
      }
      Cset += tmp->second;
   }

   if (!Cset.contains(LF.top_node())) {
      if (verbose) cout << "The given set is not a modular cut because "
                        << "it does not contain the set " << LF.face(LF.top_node()) << "."
                        << endl;
      return false;
   }

   if (!is_convex_subset(Cset, LF, verbose)) {
      if (verbose) cout << "The given set is not a modular cut because "
                        << "it is not convex."
                        << endl;
      return false;
   }

   if (!covering_condition(Cset, LF, index_of, verbose)) {
      if (verbose) cout << "The given set is not a modular cut because "
                        << "it does not satisfy the covering condition."
                        << endl;
      return false;
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
