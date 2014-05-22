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

#include "polymake/client.h"
#include "polymake/PowerSet.h"
#include "polymake/Integer.h"
#include "polymake/graph/HasseDiagram.h"
#include "polymake/Bitset.h"
#include "polymake/hash_set"
#include "polymake/list"

namespace polymake { namespace topaz {
  
Array<Set<int> > minimal_non_faces(const graph::HasseDiagram HD)
{
#if POLYMAKE_DEBUG
   const bool debug_print = perl::get_debug_level() > 1;
#endif

   const int dim = HD.dim()-1;  // dimension of the complex = HD.dim()-1
   std::list< Set<int> > min_non_faces;

   // determine the start level (the highest complete level)
   int start_dim = 1;
   while ( HD.nodes_of_dim(start_dim).size() == Integer::binom(dim,start_dim) )  ++start_dim;
   --start_dim;

   const int n_vertices=HD.node_range_of_dim(0).size(), top_node=HD.top_node();

   // iterate over all levels of HD and determine the minimal non-faces one above
   for (int d=start_dim; d<=dim; ++d) {

      // create hash set containing all faces of this dimension
      hash_set< Set<int> > faces(HD.node_range_of_dim(d).size());
      for (Entire<sequence>::const_iterator it=entire(HD.node_range_of_dim(d)); !it.at_end(); ++it)
         faces.insert(HD.face(*it));
      const hash_set< Set<int> >::iterator faces_end = faces.end();  

      // iterate over all faces of this dimension
      for (Entire<sequence>::const_iterator it=entire(HD.node_range_of_dim(d)); !it.at_end(); ++it) {
         const int n=*it;
         const Set<int>& f= HD.face(n);
         
         Bitset non_candidates(n_vertices);
         for (Entire<Graph<Directed>::out_edge_list>::const_iterator e=entire(HD.out_edges(n)); !e.at_end(); ++e)
            if (e.to_node() != top_node)
               non_candidates += (HD.face(e.to_node())-f).front();
#if POLYMAKE_DEBUG
         if (debug_print) cout << "\nface " << n << ": " << f << "\nnon_candidates: " << non_candidates << endl;
#endif
         // generate non-faces
         for (Entire<Graph<Directed>::in_edge_list>::const_iterator in_e=entire(HD.in_edges(n)); !in_e.at_end(); ++in_e) {
            for (Entire<Graph<Directed>::out_edge_list>::const_iterator out_e=entire(HD.out_edges(in_e.from_node())); !out_e.at_end(); ++out_e) {
               const int nn = out_e.to_node();
               if (n==nn)
                  continue;

               const int candidate = (HD.face(nn)-f).front();
               if ( non_candidates.contains(candidate) || candidate<f.back() )
                  continue;
#if POLYMAKE_DEBUG
               if (debug_print) cout << "candidate node " << nn << " (" << HD.face(nn) << ") with vertex: " << candidate << endl;
#endif
               // test the candidate
               bool is_minimal = true;
               for (Entire< Subsets_less_1<const Set<int>&> >::const_iterator f_it=entire(all_subsets_less_1(f)); !f_it.at_end(); ++f_it)
                  if ( faces.find(*f_it + candidate) == faces_end ) {
                     is_minimal = false;
                     break;
                  }
               if (is_minimal)  min_non_faces.push_back(f + candidate);
               non_candidates += candidate;
#if POLYMAKE_DEBUG
               if (debug_print && is_minimal) cout << "ADDING min non-face: " << f + candidate << endl;
#endif
            }
         }

      }  // end iterate over all faces of this dimension
   }

   return min_non_faces;
}

Function4perl(&minimal_non_faces, "minimal_non_faces(FaceLattice)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
