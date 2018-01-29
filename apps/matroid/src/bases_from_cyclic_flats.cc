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
#include "polymake/list"
#include "polymake/Array.h"
#include "polymake/Set.h"
#include "polymake/graph/Lattice.h"
#include "polymake/graph/Decoration.h"
#include "polymake/PowerSet.h"

namespace polymake { namespace matroid {

   using graph::Lattice;
   using graph::lattice::Sequential;
   using graph::lattice::BasicDecoration;

   /*
    * Computes all bases from the lattice of cyclic flats. A basis is a set of cardinality rank in
    * (0 .. n_elements-1) such that for all non-empty cyclic flats Z we have |B \cap Z| <= rank(Z)
    */
   Array<Set<int> > bases_from_cyclic_flats(int n_elements, int rank, perl::Object H_obj) {
      Lattice<BasicDecoration, Sequential> H(H_obj);
      const auto nelem = sequence(0,n_elements);
      const auto all_r_sets = all_subsets_of_k( nelem, rank);
      std::list<Set<int> > result;

      const NodeMap< Directed, BasicDecoration >& decor = H.decoration();
      for(const auto& B : all_r_sets) {
         bool found_witness = false;
         for(auto d_it = entire(decor); !d_it.at_end(); ++d_it) {
            if( (B * d_it->face).size() > d_it->rank) {
               found_witness = true; break;
            };
         }
         if(!found_witness) result.push_back(B);
      }
      return Array<Set<int> >(result);
   }

   Function4perl(&bases_from_cyclic_flats, "bases_from_cyclic_flats($,$, Lattice<BasicDecoration, Sequential>)");

}}
