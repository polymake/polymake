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

#ifndef POLYMAKE_MATROID_BASES_FROM_LOF_H
#define POLYMAKE_MATROID_BASES_FROM_LOF_H

#include "polymake/Array.h"
#include "polymake/Set.h"
#include "polymake/PowerSet.h"
#include "polymake/graph/Lattice.h"
#include "polymake/graph/Decoration.h"

namespace polymake { namespace matroid {

Array<Set<Int>> bases_from_lof_impl(const graph::Lattice<graph::lattice::BasicDecoration, graph::lattice::Sequential>& LF, Int n)
{
   const Int rank = LF.rank();
   if (rank == 0) {
      return Array<Set<Int>>(1);
   }
   std::vector<Set<Int>> bases;
   for (auto bit = entire(all_subsets_of_k(sequence(0, n), rank)); !bit.at_end(); ++bit) {
      const Set<Int> basis(*bit);
      bool dependent = false;
      for (Int k = rank-1; k >= 0; --k) {
         for (auto fi = entire(LF.nodes_of_rank(k)); !dependent && !fi.at_end(); ++fi) {
            if (incl(basis, LF.face(*fi)) <= 0)
               dependent = true;
         }
      }
      if (!dependent) bases.push_back(basis);
   }
   return Array<Set<Int>>(bases.size(), entire(bases));
}

} }

#endif // POLYMAKE_MATROID_BASES_FROM_LOF_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
