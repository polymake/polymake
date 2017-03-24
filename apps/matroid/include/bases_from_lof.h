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

#ifndef __POLYMAKE_MATROID_BASES_FROM_LOF_H__
#define __POLYMAKE_MATROID_BASES_FROM_LOF_H__

#include "polymake/Array.h"
#include "polymake/Set.h"
#include "polymake/PowerSet.h"
#include "polymake/graph/Lattice.h"
#include "polymake/graph/Decoration.h"

namespace polymake { namespace matroid {

   using graph::Lattice;
   using graph::lattice::Sequential;
   using graph::lattice::BasicDecoration;

Array<Set<int> > bases_from_lof_impl(const Lattice<BasicDecoration, Sequential>& LF, int n)
{
	int LF_dim = LF.rank();
	if(LF_dim == 0) {
		//THis means the rank is 0
		return Array<Set<int> >(1);
	}
   const int rank(LF_dim);
   std::vector<Set<int> > bases;
   for (Entire<Subsets_of_k<const sequence&> >::const_iterator bit=entire(all_subsets_of_k(sequence(0,n), rank)); !bit.at_end(); ++bit) {
      const Set<int> basis(*bit);
      bool dependent(false);
      for (int k=rank-1; k>=0; --k) {
         for (auto fi=entire(LF.nodes_of_rank(k)); !dependent && !fi.at_end(); ++fi) {
            if (incl(basis, LF.face(*fi)) <= 0)
               dependent = true;
         }
      }
      if (!dependent) bases.push_back(basis);
   }
   return Array<Set<int> >(bases.size(), entire(bases));
}

} }

#endif // __POLYMAKE_MATROID_BASES_FROM_LOF_H__

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
