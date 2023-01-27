/* Copyright (c) 1997-2023
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
#include "polymake/Graph.h"
#include "polymake/PowerSet.h"
#include "polymake/EquivalenceRelation.h"
#include "polymake/Bitset.h"
#include "polymake/list"

namespace polymake { namespace topaz {
  
ListReturn projectivities(BigObject p)
{
   typedef Graph<> graph;
   const graph DG = p.give("DUAL_GRAPH.ADJACENCY");
   const Array<Set<Int>> F = p.give("FACETS");
   const Int n_vert = p.give("N_VERTICES");
   const Int n_facets = F.size();
     
   EquivalenceRelation color_classes(n_vert,F[0]);  // vertices of first facets as representatives
   
   // BFS to determin color classes
   Bitset visited(n_facets);
   std::list<Int> node_queue;
   node_queue.push_back(0);
   visited+=0;
   
   while (!node_queue.empty()) {
      const Int n = node_queue.front();
      node_queue.pop_front(); 
      
      for (auto e=entire(DG.out_edges(n)); !e.at_end(); ++e) {
         const Int nn = e.to_node();
         
         if ( !visited.contains(nn) ) {
            visited+=nn;
            node_queue.push_back(nn);
         }
         
         // merge color_classes of vertices not in common ridge of n and nn
         color_classes.merge_classes( (F[n]-F[nn])+(F[nn]-F[n]) );
      }
   }
   
   // compute orbit spaces
   PowerSet<Int> orbit_spaces;
   for (auto v = entire(F[0]); !v.at_end(); ++v) {
      const Int rep = color_classes.representative(*v);

      if (*v == rep) {
         orbit_spaces += scalar2set(*v);
      } else {
         Set<Int> add;
         for (auto c=entire(orbit_spaces); !c.at_end(); ++c) 
            if (c->front()==rep) {
               add = *c;
               break;
            }
         orbit_spaces -= add;
         add.push_back(*v);
         orbit_spaces += add;
      }
   }

   ListReturn result;
   result << orbit_spaces << color_classes.representatives();
   return result;
}

Function4perl(&projectivities,"projectivities(SimplicialComplex)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
