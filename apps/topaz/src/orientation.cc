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
#include "polymake/Set.h"
#include "polymake/Array.h"
#include "polymake/Graph.h"
#include "polymake/list"
#include "polymake/hash_map"

namespace polymake { namespace topaz {
namespace {

bool consistent(const int f1, const int f2, const bool o1, const bool o2,
                const Array< Set<int> >& F,
                const Array< hash_map<int,int> >& indices)
{
   const int n1 =indices[f1].find( (F[f1]-F[f2]).front() )->second;
   const int n2 =indices[f2].find( (F[f2]-F[f1]).front() )->second;
   
   return o1==o2 ? (n1-n2)%2!=0 : (n1-n2)%2==0;
}
   
}

void orientation(perl::Object p)
{
   const Array< Set<int> > F = p.give("FACETS");
   const Graph<> DG = p.give("DUAL_GRAPH.ADJACENCY");
   const bool is_pmf = p.give("PSEUDO_MANIFOLD");
   if (!is_pmf)
      throw std::runtime_error("orientation: Complex is not a PSEUDO_MANIFOLD");

   // compute indices of the vertices for each facet
   Array< hash_map<int,int> > indices(F.size());
   int count1=0;
   for (Entire< Array< Set<int> > >::const_iterator f=entire(F);
        !f.at_end(); ++f, ++count1) {
      
      int count2=0;
      for (Entire< Set<int> >::const_iterator v=entire(*f);
           !v.at_end(); ++v, ++count2)
         indices[count1][*v] = count2;
   }
   
   Array<bool> orientation(F.size());  // 0 for not visited
   orientation[0] = true;
   std::list<int> node_queue;
   node_queue.push_back(0);
   bool orientable = true;
   
   while (!node_queue.empty() && orientable) {
      const int n=node_queue.front();
      node_queue.pop_front();

      for (Entire< Graph<>::out_edge_list >::const_iterator e=entire(DG.out_edges(n));
           !e.at_end(); ++e) {
         const int nn=e.to_node();
         
         if (!orientation[nn]) {  // nn not been visited yet, compute orientation
            orientation[nn]= consistent(n,nn,orientation[n],true,F,indices);
            node_queue.push_back(nn);
            
         } else  // check consistency of orientation
            if ( n<nn && !consistent(n,nn,orientation[n],orientation[nn],F,indices) ) {
               orientable = false;
               break;
            }
      }
   }
   p.take("ORIENTED_PSEUDO_MANIFOLD") << orientable;
   if (orientable)
      p.take("ORIENTATION") << orientation;
   else p.take("ORIENTATION") << perl::undefined();
}

Function4perl(&orientation,"orientation");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
