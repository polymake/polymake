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

#include "polymake/client.h"
#include "polymake/graph/Lattice.h"
#include "polymake/graph/Decoration.h"
#include "polymake/IndexedSubgraph.h"
#include "polymake/Map.h"

namespace polymake { namespace matroid {

   using graph::Lattice;
   using graph::lattice::Sequential;
   using graph::lattice::BasicDecoration;

   /* *@struct CompareByRank
    *   @brief Compares two Sets of ints by comparing their Ranks
    */
   class CompareByRank
   {
      private:
         const Map<Set<int>,int>& RankMap;

      public:
         CompareByRank(const Map<Set<int>,int>& RankMap_arg) :
            RankMap(RankMap_arg)
      {}

         pm::cmp_value operator() (const Set<int>& a, const Set<int>& b) const
         {
            if (RankMap[a] != RankMap[b])
               return operations::cmp()( RankMap[a],RankMap[b] );
            return operations::cmp()( a,b );
         }
   };

   perl::Object matroid_from_cyclic_flats(const Array<Set<int>>& Faces, const Array<int>& Ranks, const int& n_elements)
   {
      Lattice<BasicDecoration, Sequential> LF;

      if(Faces.size() > 0) {
         //sort the sets w.r.t. their ranks
         Map<Set<int>,int> RankMap;
         for (int i=0; i<Faces.size(); ++i)
            RankMap[Faces[i]] = Ranks[i];
         CompareByRank cmp(RankMap);
         Set<Set<int>,CompareByRank> sorted_sets(cmp);
         for (const auto s : Faces) {
            sorted_sets+=s;
         }

         int rank = RankMap[sorted_sets.front()];
         for (Entire<Set<Set<int>, CompareByRank>>::const_iterator it=entire(sorted_sets); !(it.at_end()); ++it) {
            rank = RankMap[*it];
            LF.add_node(BasicDecoration(*it, rank));

            //check in the induced graph of included faces for transitivity relations
            Set<int> incl_set_indices;
            int index = 0;
            for (Entire<Set<Set<int>, CompareByRank>>::const_iterator sub_it=entire(sorted_sets); sub_it != it && RankMap[*sub_it] < rank ; ++sub_it, ++index)
               if (incl(*sub_it,*it) == -1)
                  incl_set_indices+=index;
            auto indg = induced_subgraph(LF.graph(),incl_set_indices);
            for (auto from_node=entire(nodes(indg)); !(from_node.at_end()); ++from_node)
               if (indg.out_degree(*from_node) == 0)
                  LF.add_edge(*from_node,LF.graph().nodes()-1);
         }
      }

      perl::Object m("Matroid");
      m.take("N_ELEMENTS") << n_elements;
      m.take("LATTICE_OF_CYCLIC_FLATS") << LF.makeObject();

      return m;
   }

   UserFunction4perl("# @category Producing a matroid from other objects"
         "# Computes the face lattice of the given sets by inclusion."
         "# @param Array<Set<int>> F faces of the lattice of cyclic flats"
         "# @param Array<Set<int>> R ranks of the faces"
         "# @param Int N number of elements"
         "# @return Matroid matroid with the specified lattice of cylcic flats",
         &matroid_from_cyclic_flats, "matroid_from_cyclic_flats(Array<Set<Int>>, Array<Int>, Int)");


} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
