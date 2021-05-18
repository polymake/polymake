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
#include "polymake/PowerSet.h"
#include "polymake/Map.h"
#include "polymake/graph/Lattice.h"
#include "polymake/graph/Decoration.h"
#include "polymake/graph/LatticeTools.h"
#include <list>

namespace polymake { namespace matroid {

template <typename HDType>
bool covering_condition(const Set<Int>& Cset, const HDType& LF, const Map<Set<Int>, Int>& index_of, bool verbose)
{
   for (auto pit = entire(all_subsets_of_k(Cset, 2)); !pit.at_end(); ++pit) {
      const Set<Int> p(*pit);
      const Int x(p.front()), y(p.back());
      const Int join = index_of[LF.face(x) * LF.face(y)];
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

/*
  A modular cut is a subset C of a lattice of flats such that
  (1) C is convex, i.e.  x,z in C, x<y<z  implies  y in C
  (2) C contains {0,...,n-1}
  (3) x,y in C,  x covers x^y  implies  x^y in C.
 */
template<typename SetType>
bool is_modular_cut_impl(const Array<SetType>& C, const graph::Lattice<graph::lattice::BasicDecoration, graph::lattice::Sequential>& LF, bool verbose)
{
   // prepare data structures for lattice of flats
   Map<Set<Int>, Int> index_of;
   for (Int i = 0; i <= LF.rank(); ++i) {
      for (auto fit = entire(LF.nodes_of_rank(i)); !fit.at_end(); ++fit) {
         index_of[LF.face(*fit)] = *fit;
      }
   }

   Set<Int> Cset;
   for (auto cit = entire(C); !cit.at_end(); ++cit) {
      auto tmp = index_of.find(*cit);
      if (tmp == index_of.end()) {
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


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
