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
#include "polymake/graph/HasseDiagram.h"

namespace polymake { namespace matroid {


Array<Set<int> > bases_from_lof_impl(const graph::HasseDiagram& LF, int n)
{
   const int rank(LF.dim());
   std::vector<Set<int> > bases;
   for (Entire<Subsets_of_k<const sequence&> >::const_iterator bit=entire(all_subsets_of_k(sequence(0,n), rank)); !bit.at_end(); ++bit) {
      const Set<int> basis(*bit);
      bool dependent(false);
      for (int k=rank-1; k>=0; --k) {
         for (Entire<graph::HasseDiagram::nodes_of_dim_set>::iterator fi=entire(LF.node_range_of_dim(k)); !dependent && !fi.at_end(); ++fi) {
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
