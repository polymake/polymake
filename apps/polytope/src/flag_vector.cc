/* Copyright (c) 1997-2014
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

/** @file flag_vector
 *  Calculate (the non-redundant part of) the flag vector of a polytope.
 *
 *  @author Axel Werner
 */

#include "polymake/client.h"
#include "polymake/Vector.h"
#include "polymake/graph/HasseDiagram.h"
#include "polymake/Fibonacci.h"
#include "polymake/Integer.h"

namespace polymake { namespace polytope {
namespace {

typedef Graph<Directed> PartialLattice;
typedef NodeMap<Directed,Integer> IncidenceMap;

Integer* calcEntry(const graph::HasseDiagram& F, PartialLattice& G, IncidenceMap& Inc, int k, Integer* fl)
{
   // INVARIANT for G here: layer k is connected to some upper layer, all layers below k have no edges at all
   Integer Entry=0;
   for (Entire<sequence>::iterator k_node=entire(F.node_range_of_dim(k)); !k_node.at_end(); ++k_node)
      Entry += (Inc[*k_node] = accumulate(select(Inc, G.out_adjacent_nodes(*k_node)), operations::add()));

   int i=k-2;
   if (i>=0) {
      // connect layer k-2 with layer k
      for (Entire<sequence>::iterator i_node=entire(F.node_range_of_dim(i)); !i_node.at_end(); ++i_node)
         // iterate over the adjacent nodes in the layer between (==k-1)
         for (PartialLattice::out_adjacent_node_list::const_iterator btw_node=F.out_adjacent_nodes(*i_node).begin(); !btw_node.at_end(); ++btw_node)
            G.out_adjacent_nodes(*i_node) += F.out_adjacent_nodes(*btw_node);

      for (;;) {
         fl=calcEntry(F,G,Inc,i,fl);
         if (i==0) break;
         // move the edges from layer i to layer i-1
         for (Entire<sequence>::iterator i_node=entire(F.node_range_of_dim(i)); !i_node.at_end(); ++i_node) {
            for (PartialLattice::in_adjacent_node_list::const_iterator down_node=F.in_adjacent_nodes(*i_node).begin(); !down_node.at_end(); ++down_node)
               G.out_adjacent_nodes(*down_node) += G.out_adjacent_nodes(*i_node);
            G.out_edges(*i_node).clear();
         }
         --i;
      }
      // remove all edges betwen layers 0 and k, thus restoring the entry invariant
      for (Entire<sequence>::const_iterator k_node=entire(F.node_range_of_dim(k)); !k_node.at_end(); ++k_node)
         G.in_edges(*k_node).clear();
   }
   *--fl=Entry;
   return fl;
}
} // end anonymous namespace

Vector<Integer> flag_vector(perl::Object HD_obj)
{
   const graph::HasseDiagram F(HD_obj);
   const int d = F.dim();
   PartialLattice G(F.nodes());
   IncidenceMap Inc(G);
   // provide for Inc[TOP]==1 avoiding extra conditionals in the recursive part
   G.edge(F.top_node(), F.bottom_node()); Inc[F.bottom_node()]=1;
   Vector<Integer> fl(fibonacci_number(d));
   calcEntry(F,G,Inc,d,fl.end());
   return fl;
}

Function4perl(&flag_vector, "flag_vector(FaceLattice)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
