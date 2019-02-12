/*
   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA  02110-1301, USA.

   ---
   Copyright (C) 2011 - 2015, Simon Hampe <simon.hampe@googlemail.com>

   Computations on nested matroids
   */

#include "polymake/client.h"
#include "polymake/Array.h"
#include "polymake/Set.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Vector.h"
#include "polymake/linalg.h"
#include "polymake/Map.h"
#include "polymake/graph/LatticeTools.h"
#include "polymake/graph/Decoration.h"
#include "polymake/PowerSet.h"
#include "polymake/tropical/cyclic_chains.h"

namespace polymake { namespace tropical {

   using graph::Lattice;
   using graph::lattice::Sequential;
   using graph::lattice::BasicDecoration;

   class CompareBySize {
      public:
         pm::cmp_value operator() (const Set<int>&a, const Set<int>&b) const {
            return operations::cmp()(a.size(), b.size());
         }
   };

   /*
    * @brief Computes the maximal transversal presentation of a nested matroid from its chain of
    * cyclic flats
    * @param int n The size of the ground set
    * @param Array<Set<int> > flats The cyclic flats, ordered from smallest to largest
    * @param Array<int> ranks The ranks of the corresponding flats
    */
   IncidenceMatrix<> presentation_from_chain(int n, const IncidenceMatrix<>& flats, const Array<int> ranks) {
      Set<int> coloops = sequence(0, n) - flats[flats.rows()-1];
      int total_rank = coloops.size() + ranks[ranks.size()-1];
      IncidenceMatrix<> result(total_rank, n);

      //First: coloops as complements of largest cyclic flat
      int current_index = 0;
      for(int i = 0; i < coloops.size(); i++,current_index++) {
         result[i] = coloops;
      }

      //Move backwards in the list of flats
      for(int j =  flats.rows()-2; j >= 0; j--) {
         Set<int> complement = sequence(0,n) - flats[j];
         int occ = ranks[j+1] - ranks[j];
         for(int k = 0; k < occ; k++, current_index++) {
            result[current_index] = complement;
         }
      }

      return result;
   }

   /*
    * @brief Converts a loopfree nested matroid, given in terms of its maximal
    * transversal presentation, into a list of cyclic flats and their ranks.
    * The presentation is assumed to be ordered from smallest to largest set.
    * @param IncidenceMatrix<> presentation The maximal transversal presentation.
    * @return Map<Set<int>, int> Maps cyclic flats to their ranks.
    */
   Map<Set<int>, int> cyclic_flats_from_presentation(const IncidenceMatrix<> &presentation) {
      if(presentation.rows() == 0) {
         return Map<Set<int>, int>();
      }
      int n = presentation.cols();
      int r = presentation.rows();
      int current_row_index = 0;
      Vector<Set<int> > flats;
      Vector<int> occurences;

      while(current_row_index < r) {
         Set<int> current_set = presentation.row(current_row_index);
         int current_count = 1;
         //Count how many times this row occurs
         while( current_set == (current_row_index < r-1? presentation.row(current_row_index+1) : Set<int>())) {
            current_count++;
            current_row_index++;
         }
         flats |= (sequence(0,n) - current_set);
         occurences |= current_count;
         current_row_index++;
      }

      //If there are no coloops the full set is missing as a cyclic flat
      if(occurences[0] < presentation.row(0).size()) {
         flats = ( sequence(0,n) ) | flats;
         occurences = 0 | occurences;
      }

      Map<Set<int>, int> result;
      int last_rank = r; int flat_index = 0;
      for (auto f_it = entire(flats); !f_it.at_end(); ++f_it, ++flat_index) {
         last_rank -= occurences[flat_index];
         result[*f_it] = last_rank;
      }

      return result;
   }


   //Compute a representation of a matroid in the basis of nested matroids
   //in the matroid intersection ring.
   perl::ListReturn matroid_nested_decomposition(perl::Object matroid) {
      int n = matroid.give("N_ELEMENTS");
      perl::Object flats = matroid.give("LATTICE_OF_CYCLIC_FLATS");
      Lattice<BasicDecoration, Sequential> flats_lattice(flats);
      IncidenceMatrix<> cyclic_flats = flats.give("FACES");
      Lattice<BasicDecoration> chains = cyclic_chains(flats_lattice);
      Vector<int> coefficients = top_moebius_function(chains);
      Set<int> supp = support( coefficients) - chains.top_node();

      //If it has loops, it's the zero element
      if( flats_lattice.face(flats_lattice.bottom_node()).size() > 0) {
         perl::ListReturn result;
         result << Array<IncidenceMatrix<> >();
         result << Array<int>();
         return result;
      }

      Array<IncidenceMatrix<> > nested_presentations(supp.size());
      Array<int> final_coefficients(supp.size());

      int current_index = 0;
      for (auto s = entire(supp); !s.at_end(); ++s, ++current_index) {
         final_coefficients[current_index] = -coefficients[*s];
         auto chain_nodes = select(flats_lattice.decoration(), chains.face(*s));
         Set<Set<int>, CompareBySize> ordered_faces_list( entire(
                  attach_member_accessor( chain_nodes,
                     ptr2type< BasicDecoration, Set<int>, &BasicDecoration::face>())
                  ));
         Set<int> ordered_ranks_list( entire(
                  attach_member_accessor( chain_nodes,
                     ptr2type< BasicDecoration, int, &BasicDecoration::rank>())
                  ));
         IncidenceMatrix<> ordered_faces( ordered_faces_list.size(), n, entire(ordered_faces_list));
         Array<int> ordered_ranks( ordered_ranks_list.size(), entire(ordered_ranks_list));
         nested_presentations[current_index] = presentation_from_chain(n, ordered_faces, ordered_ranks);
      }

      perl::ListReturn result;
         result << nested_presentations;
         result << final_coefficients;

      return result;
   }

   /*
    * @brief Constructs a loopfree nested matroid from its maximal transversal presentation
    * @param IncidenceMatrix<> The maximal transversal presentation. Assumed to be
    * ordered from smallest to largest set. Also, the largest set has to be the full set.
    * @param int n Size of the ground set.
    * @return matroid::Matroid
    */
   perl::Object nested_matroid_from_presentation(const IncidenceMatrix<> &presentation, int n) {
      int r = presentation.rows();

      Map<Set<int>, int> cyclic_flats = cyclic_flats_from_presentation(presentation);
      Vector<Set<int> > bases(all_subsets_of_k( sequence(0,n),r));

      //Remove bases that contain too much of a cyclic flat
      for (auto cf_it = entire(cyclic_flats); !cf_it.at_end(); ++cf_it) {
         Set<int> bad_bases;
         int base_index =0;
         for (auto b_it = entire(bases); !b_it.at_end(); ++b_it, ++base_index) {
            if (((*b_it) * cf_it->first).size() > cf_it->second)
              bad_bases += base_index;
         }
         bases = bases.slice(~bad_bases);
      }

      perl::Object result("matroid::Matroid");
         result.take("N_ELEMENTS") << n;
         result.take("BASES") << bases;
      return result;
   }

   Function4perl(&presentation_from_chain, "presentation_from_chain($, $,$)");

   Function4perl(&matroid_nested_decomposition, "matroid_nested_decomposition(matroid::Matroid)");

   Function4perl(&nested_matroid_from_presentation, "nested_matroid_from_presentation(IncidenceMatrix, $)");
}}
