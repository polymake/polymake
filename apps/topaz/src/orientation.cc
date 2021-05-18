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
#include "polymake/Set.h"
#include "polymake/Array.h"
#include "polymake/Graph.h"
#include "polymake/list"
#include "polymake/hash_map"

namespace polymake { namespace topaz {

void orientation(BigObject p)
{
   const Array<Set<Int>> F = p.give("FACETS");
   const Graph<> DG = p.give("DUAL_GRAPH.ADJACENCY");
   const bool is_pmf = p.give("PSEUDO_MANIFOLD");
   if (!is_pmf)
      throw std::runtime_error("orientation: Complex is not a PSEUDO_MANIFOLD");

   Array<bool> orientation(F.size()); 
   Array<bool> visited(F.size());
   orientation[0] = true; visited[0] = true;
   std::list<Int> node_queue;
   node_queue.push_back(0);
   bool orientable = true;
   
   while (!node_queue.empty() && orientable) {
      const Int n = node_queue.front();
      node_queue.pop_front();

      for (auto e=entire(DG.out_edges(n)); !e.at_end(); ++e) {
         const Int nn = e.to_node();
         
         if (!visited[nn]) {  // nn has not been visited yet, compute orientation
            orientation[nn]= !orientation[n];
            node_queue.push_back(nn);
            visited[nn] = true;
         } else  // check consistency of orientation
            if (orientation[nn] == orientation[n]) {
               orientable = false;
               break;
            }
      }
   }
   p.take("ORIENTED_PSEUDO_MANIFOLD") << orientable;
   if (orientable)
      p.take("ORIENTATION") << orientation;
   else
      p.take("ORIENTATION") << Undefined();
}

Function4perl(&orientation,"orientation");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
