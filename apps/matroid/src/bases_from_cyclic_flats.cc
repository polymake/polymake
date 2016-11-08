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
#include "polymake/list"
#include "polymake/Array.h"
#include "polymake/Set.h"
#include "polymake/graph/HasseDiagram.h"
#include "polymake/PowerSet.h"

namespace polymake { namespace matroid {

   using graph::HasseDiagram;

   /*
    * Computes all bases from the lattice of cyclic flats. A basis is a set of cardinality rank in 
    * (0 .. n_elements-1) such that for all cyclic flats Z we have |B \cap Z| < rank(Z).
    */
   Array<Set<int> > bases_from_cyclic_flats(int n_elements, int rank, HasseDiagram H) {
      const auto nelem = sequence(0,n_elements);
      const auto all_r_sets = all_subsets_of_k( nelem, rank);
      std::list<Set<int> > result;

      const NodeMap< Directed, Set<int> >& faces = H.faces();
      const int Hdim = H.dim();
      const auto& bottom_set = faces[H.bottom_node()];
      for(const auto& B : all_r_sets) {
         bool found_witness = false;
         if(!(B * bottom_set).empty()) {
            found_witness = true; 
         }
         for(int d = 0; d <= Hdim; d++) {
            const auto node_indices = H.nodes_of_dim(d);
            for(const auto& ni : node_indices) {
               if( (B * faces[ni]).size() > d+1) {
                  found_witness = true; break;
               }
            }
            if(found_witness) break;
         }
         if(!found_witness) result.push_back(B);
      }
      return Array<Set<int> >(result);
   }

   Function4perl(&bases_from_cyclic_flats, "bases_from_cyclic_flats($,$, FaceLattice)");

}}
