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

   ---
   Copyright (c) 2016-2020
   Ewgenij Gawrilow, Michael Joswig, and the polymake team
   Technische Universität Berlin, Germany
   https://polymake.org

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
  pm::cmp_value operator() (const Set<Int>& a, const Set<Int>& b) const
  {
    return operations::cmp()(a.size(), b.size());
  }
};

/*
 * @brief Computes the maximal transversal presentation of a nested matroid from its chain of
 * cyclic flats
 * @param Int n The size of the ground set
 * @param Array<Set<Int> > flats The cyclic flats, ordered from smallest to largest
 * @param Array<Int> ranks The ranks of the corresponding flats
 */
IncidenceMatrix<> presentation_from_chain(Int n, const IncidenceMatrix<>& flats, const Array<Int>& ranks)
{
  Set<Int> coloops = sequence(0, n) - flats[flats.rows()-1];
  Int total_rank = coloops.size() + ranks[ranks.size()-1];
  IncidenceMatrix<> result(total_rank, n);

  // First: coloops as complements of largest cyclic flat
  Int current_index = 0;
  for (Int i = 0; i < coloops.size(); i++,current_index++) {
    result[i] = coloops;
  }

  // Move backwards in the list of flats
  for (Int j =  flats.rows()-2; j >= 0; j--) {
    Set<Int> complement = sequence(0,n) - flats[j];
    Int occ = ranks[j+1] - ranks[j];
    for (Int k = 0; k < occ; k++, current_index++) {
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
 * @return Map<Set<int>, Int> Maps cyclic flats to their ranks.
 */
Map<Set<Int>, Int> cyclic_flats_from_presentation(const IncidenceMatrix<>& presentation)
{
  if (presentation.rows() == 0) {
    return Map<Set<Int>, Int>();
  }
  Int n = presentation.cols();
  Int r = presentation.rows();
  Int current_row_index = 0;
  Vector<Set<Int>> flats;
  Vector<Int> occurences;

  while (current_row_index < r) {
    Set<Int> current_set = presentation.row(current_row_index);
    Int current_count = 1;
    // Count how many times this row occurs
    while (current_row_index < r-1 ? current_set == presentation.row(current_row_index+1) : current_set.empty()) {
      ++current_count;
      ++current_row_index;
    }
    flats |= (sequence(0,n) - current_set);
    occurences |= current_count;
    ++current_row_index;
  }

  // If there are no coloops the full set is missing as a cyclic flat
  if (occurences[0] < presentation.row(0).size()) {
    flats = ( sequence(0,n) ) | flats;
    occurences = 0 | occurences;
  }

  Map<Set<Int>, Int> result;
  Int last_rank = r;
  Int flat_index = 0;
  for (auto f_it = entire(flats); !f_it.at_end(); ++f_it, ++flat_index) {
    last_rank -= occurences[flat_index];
    result[*f_it] = last_rank;
  }

  return result;
}


// Compute a representation of a matroid in the basis of nested matroids
// in the matroid intersection ring.
ListReturn matroid_nested_decomposition(BigObject matroid)
{
  Int n = matroid.give("N_ELEMENTS");
  BigObject flats = matroid.give("LATTICE_OF_CYCLIC_FLATS");
  Lattice<BasicDecoration, Sequential> flats_lattice(flats);
  IncidenceMatrix<> cyclic_flats = flats.give("FACES");
  Lattice<BasicDecoration> chains = cyclic_chains(flats_lattice);
  Vector<Int> coefficients = top_moebius_function(chains);
  Set<Int> supp = support( coefficients) - chains.top_node();

  // If it has loops, it's the zero element
  if (flats_lattice.face(flats_lattice.bottom_node()).size() > 0) {
    ListReturn result;
    result << Array<IncidenceMatrix<> >();
    result << Array<Int>();
    return result;
  }

  Array<IncidenceMatrix<>> nested_presentations(supp.size());
  Array<Int> final_coefficients(supp.size());

  Int current_index = 0;
  for (auto s = entire(supp); !s.at_end(); ++s, ++current_index) {
    final_coefficients[current_index] = -coefficients[*s];
    auto chain_nodes = select(flats_lattice.decoration(), chains.face(*s));
    Set<Set<Int>, CompareBySize> ordered_faces_list( entire(
                  attach_member_accessor( chain_nodes,
                     ptr2type< BasicDecoration, Set<Int>, &BasicDecoration::face>())
                  ));
    Set<Int> ordered_ranks_list( entire(
                  attach_member_accessor( chain_nodes,
                     ptr2type< BasicDecoration, Int, &BasicDecoration::rank>())
                  ));
    IncidenceMatrix<> ordered_faces(ordered_faces_list.size(), n, entire(ordered_faces_list));
    Array<Int> ordered_ranks( ordered_ranks_list.size(), entire(ordered_ranks_list));
    nested_presentations[current_index] = presentation_from_chain(n, ordered_faces, ordered_ranks);
  }

  ListReturn result;
  result << nested_presentations;
  result << final_coefficients;

  return result;
}

/*
 * @brief Constructs a loopfree nested matroid from its maximal transversal presentation
 * @param IncidenceMatrix<> The maximal transversal presentation. Assumed to be
 * ordered from smallest to largest set. Also, the largest set has to be the full set.
 * @param Int n Size of the ground set.
 * @return matroid::Matroid
 */
BigObject nested_matroid_from_presentation(const IncidenceMatrix<> &presentation, Int n)
{
  Int r = presentation.rows();

  Map<Set<Int>, Int> cyclic_flats = cyclic_flats_from_presentation(presentation);
  Vector<Set<Int>> bases(all_subsets_of_k(sequence(0,n), r));

  // Remove bases that contain too much of a cyclic flat
  for (auto cf_it = entire(cyclic_flats); !cf_it.at_end(); ++cf_it) {
    Set<Int> bad_bases;
    Int base_index = 0;
    for (auto b_it = entire(bases); !b_it.at_end(); ++b_it, ++base_index) {
      if (((*b_it) * cf_it->first).size() > cf_it->second)
        bad_bases += base_index;
    }
    bases = bases.slice(~bad_bases);
  }

  BigObject result("matroid::Matroid");
  result.take("N_ELEMENTS") << n;
  result.take("BASES") << bases;
  return result;
}

Function4perl(&presentation_from_chain, "presentation_from_chain($, $,$)");

Function4perl(&matroid_nested_decomposition, "matroid_nested_decomposition(matroid::Matroid)");

Function4perl(&nested_matroid_from_presentation, "nested_matroid_from_presentation(IncidenceMatrix, $)");

} }
