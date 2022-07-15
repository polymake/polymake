/* Copyright (c) 1997-2022
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

/** @file flag_vector
 *  Calculate (the non-redundant part of) the flag vector of a polytope.
 *
 *  @author Axel Werner
 */

#include "polymake/client.h"
#include "polymake/Vector.h"
#include "polymake/graph/Lattice.h"
#include "polymake/graph/Decoration.h"
#include "polymake/Fibonacci.h"
#include "polymake/Integer.h"

namespace polymake { namespace polytope {
namespace {

typedef Graph<Directed> PartialLattice;
typedef NodeMap<Directed,Integer> IncidenceMap;

Vector<Integer>::iterator calcEntry(const graph::Lattice<graph::lattice::BasicDecoration, graph::lattice::Sequential>& F, PartialLattice& G, IncidenceMap& Inc, Int k, Vector<Integer>::iterator fl)
{
   // INVARIANT for G here: layer k is connected to some upper layer, all layers below k have no edges at all
   Integer Entry = 0;
   for (const auto k_node : F.nodes_of_rank(k+1))
      Entry += (Inc[k_node] = accumulate(select(Inc, G.out_adjacent_nodes(k_node)), operations::add()));

   Int i = k-2;
   if (i >= 0) {
      // connect layer k-2 with layer k
      for (const auto i_node : F.nodes_of_rank(i+1))
         // iterate over the adjacent nodes in the layer between (==k-1)
         for (const auto btw_node : F.out_adjacent_nodes(i_node))
            G.out_adjacent_nodes(i_node) += F.out_adjacent_nodes(btw_node);

      for (;;) {
         fl = calcEntry(F, G, Inc, i, fl);
         if (i == 0) break;
         // move the edges from layer i to layer i-1
         for (const auto i_node : F.nodes_of_rank(i+1)) {
            for (const auto down_node : F.in_adjacent_nodes(i_node))
               G.out_adjacent_nodes(down_node) += G.out_adjacent_nodes(i_node);
            G.out_edges(i_node).clear();
         }
         --i;
      }
      // remove all edges betwen layers 0 and k, thus restoring the entry invariant
      for (const auto k_node : F.nodes_of_rank(k+1))
         G.in_edges(k_node).clear();
   }
   *--fl = Entry;
   return fl;
}

} // end anonymous namespace

Vector<Integer> flag_vector(BigObject HD_obj)
{
   const graph::Lattice<graph::lattice::BasicDecoration, graph::lattice::Sequential> F(HD_obj);
   const Int d = F.rank();
   PartialLattice G(F.nodes());
   IncidenceMap Inc(G);
   // provide for Inc[TOP]==1 avoiding extra conditionals in the recursive part
   G.edge(F.top_node(), F.bottom_node());
   Inc[F.bottom_node()] = 1;
   Vector<Integer> fl(static_cast<Int>(Integer::fibonacci(d)));
   calcEntry(F, G, Inc, d-1, fl.end());
   return fl;
}

Function4perl(&flag_vector, "flag_vector(Lattice<BasicDecoration, Sequential>)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
