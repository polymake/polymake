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
#include "polymake/Graph.h"
#include "polymake/PowerSet.h"
#include "polymake/EquivalenceRelation.h"
#include "polymake/Bitset.h"
#include "polymake/list"

namespace polymake { namespace topaz {
  
perl::ListReturn projectivities(perl::Object p)
{
   typedef Graph<> graph;
   const graph DG = p.give("DUAL_GRAPH.ADJACENCY");
   const Array< Set<int> > F = p.give("FACETS");
   const int n_vert = p.give("N_VERTICES");
   const int n_facets = F.size();
     
   EquivalenceRelation color_classes(n_vert,F[0]);  // vertices of first facets as representatives
   
   // BFS to determin color classes
   Bitset visited(n_facets);
   std::list<int> node_queue;
   node_queue.push_back(0);
   visited+=0;
   
   while (!node_queue.empty()) {
      const int n=node_queue.front();
      node_queue.pop_front(); 
      
      for (auto e=entire(DG.out_edges(n)); !e.at_end(); ++e) {
         const int nn = e.to_node();
         
         if ( !visited.contains(nn) ) {
            visited+=nn;
            node_queue.push_back(nn);
         }
         
         // merge color_classes of vertices not in common ridge of n and nn
         color_classes.merge_classes( (F[n]-F[nn])+(F[nn]-F[n]) );
      }
   }
   
   // compute orbit spaces
   PowerSet<int> orbit_spaces;
   for (auto v=entire(F[0]); !v.at_end(); ++v) {
      const int rep = color_classes.representative(*v);

      if (*v == rep)
         orbit_spaces += scalar2set(*v);
      else {
         Set<int> add;
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

   perl::ListReturn result;
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
