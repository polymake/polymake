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

#include "polymake/client.h"
#include "polymake/graph/HasseDiagram.h"
#include "polymake/Matrix.h"
#include "polymake/Integer.h"

namespace polymake { namespace graph {

Matrix<Integer> f2_vector(perl::Object p)
{
   const HasseDiagram HD(p);
   const int d=HD.dim();

   Matrix<Integer> F2(d,d);
   if(d == 0) return F2;
   F2(0,0) = HD.node_range_of_dim(0).size();
   if (d>1) {
      Graph<Directed> G(HD.graph());
      for (int i=1;;) {
         /* Loop invariant:
          * All edges starting in layers 0..i-1 end in the layer i
          */
         for (int j=0; j<i; ++j) {
            Integer cnt(0);
            for (Entire<sequence>::iterator f=entire(HD.node_range_of_dim(j)); !f.at_end(); ++f)
               cnt += G.out_degree(*f);
            F2(i,j)=F2(j,i)=cnt;
         }

         const sequence this_layer=HD.node_range_of_dim(i);
         F2(i,i) = this_layer.size();
         if (++i>=d) break;

         for (Entire<sequence>::const_iterator this_layer_node=entire(this_layer);
              !this_layer_node.at_end();  ++this_layer_node) {
            Graph<Directed>::in_adjacent_node_list_ref in_edges=G.in_adjacent_nodes(*this_layer_node);
            for (Graph<Directed>::out_adjacent_node_list::iterator next_layer_node=G.out_adjacent_nodes(*this_layer_node).begin();
                 !next_layer_node.at_end();  ++next_layer_node)
               G.in_adjacent_nodes(*next_layer_node) += in_edges;
            in_edges.clear();
         }
      }
   }

   return F2;
}

Function4perl(&f2_vector, "f2_vector(FaceLattice)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
