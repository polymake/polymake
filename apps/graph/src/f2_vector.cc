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

#include "polymake/client.h"
#include "polymake/graph/Lattice.h"
#include "polymake/Matrix.h"
#include "polymake/Integer.h"

namespace polymake { namespace graph {

template <typename Decoration, typename SeqType>
Matrix<Integer> f2_vector(BigObject p)
{
   const Lattice<Decoration, SeqType> HD(p);
   const Int d = HD.rank()-1;

   Matrix<Integer> F2(d,d);
   if (d == 0) return F2;
   F2(0,0) = HD.nodes_of_rank(1).size();
   if (d>1) {
      Graph<Directed> G(HD.graph());
      for (Int i = 1; ;) {
         // Loop invariant:
         // All edges starting in layers 0..i-1 end in the layer i
         for (Int j = 0; j < i; ++j) {
            Integer cnt(0);
            for (auto f=entire(HD.nodes_of_rank(j+1)); !f.at_end(); ++f)
               cnt += G.out_degree(*f);
            F2(i,j)=F2(j,i)=cnt;
         }

         const auto this_layer=HD.nodes_of_rank(i+1);
         F2(i,i) = this_layer.size();
         if (++i>=d) break;

         for (const auto this_layer_node : this_layer) {
            auto&& in_edges=G.in_adjacent_nodes(this_layer_node);
            for (const auto next_layer_node : G.out_adjacent_nodes(this_layer_node))
               G.in_adjacent_nodes(next_layer_node) += in_edges;
            in_edges.clear();
         }
      }
   }

   return F2;
}

FunctionTemplate4perl("f2_vector<Decoration, SeqType>(Lattice<Decoration, SeqType>)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
