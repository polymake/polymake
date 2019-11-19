/* Copyright (c) 1997-2019
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
#include "polymake/IndexedSubgraph.h"
#include "polymake/Map.h"

namespace polymake { namespace matroid {

using graph::Lattice;
using graph::lattice::Sequential;
using graph::lattice::BasicDecoration;

class CompareByRank
{
private:
   const Map<Set<int>,int>& RankMap;

public:
   CompareByRank(const Map<Set<int>, int>& RankMap_arg)
      : RankMap(RankMap_arg) {}

   pm::cmp_value operator() (const Set<int>& a, const Set<int>& b) const
   {
      pm::cmp_value result = operations::cmp()(RankMap[a], RankMap[b]);
      if (result == pm::cmp_eq) result = operations::cmp()(a, b);
      return result;
   }
};

perl::Object matroid_from_cyclic_flats(const Array<Set<int>>& Faces, const Array<int>& Ranks, const int n_elements)
{
   Lattice<BasicDecoration, Sequential> LF;

   if (!Faces.empty()) {
      // sort the sets w.r.t. their ranks
      Map<Set<int>, int> RankMap;
      for (int i = 0, n = Faces.size(); i < n; ++i)
         RankMap[Faces[i]] = Ranks[i];
      Set<Set<int>, CompareByRank> sorted_sets(CompareByRank{RankMap});
      for (const auto s : Faces) {
         sorted_sets += s;
      }

      for (auto it=entire(sorted_sets); !(it.at_end()); ++it) {
         const int rank = RankMap[*it];
         LF.add_node(BasicDecoration(*it, rank));

         // check in the induced graph of included faces for transitivity relations
         Set<int> incl_set_indices;
         int index = 0;
         for (auto sub_it=entire(sorted_sets); sub_it != it && RankMap[*sub_it] < rank ; ++sub_it, ++index)
            if (incl(*sub_it,*it) == -1)
               incl_set_indices += index;
         auto indg = induced_subgraph(LF.graph(),incl_set_indices);
         for (auto from_node=entire(nodes(indg)); !(from_node.at_end()); ++from_node)
            if (indg.out_degree(*from_node) == 0)
               LF.add_edge(*from_node,LF.graph().nodes()-1);
      }
   }

   perl::Object m("Matroid");
   m.take("N_ELEMENTS") << n_elements;
   m.take("LATTICE_OF_CYCLIC_FLATS") << LF;

   return m;
}

UserFunction4perl("# @category Producing a matroid from other objects"
                  "# Computes the face lattice of the given sets by inclusion."
                  "# @param Array<Set<Int>> F faces of the lattice of cyclic flats"
                  "# @param Array<Set<Int>> R ranks of the faces"
                  "# @param Int N number of elements"
                  "# @return Matroid matroid with the specified lattice of cylcic flats",
                  &matroid_from_cyclic_flats, "matroid_from_cyclic_flats(Array<Set<Int>>, Array<Int>, Int)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
