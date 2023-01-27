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
#include "polymake/Array.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Set.h"
#include "polymake/Graph.h"
#include "polymake/graph/Lattice.h"
#include "polymake/graph/Decoration.h"

namespace polymake { namespace matroid {

using graph::Lattice;
using graph::lattice::Sequential;
using graph::lattice::BasicDecoration;

// Checks whether a matroid is nested, i.e. its lattice of cyclic flats is a chain.
bool is_nested(BigObject matroid)
{
  BigObject cgraph_obj = matroid.give("LATTICE_OF_CYCLIC_FLATS");
  const Lattice<BasicDecoration, Sequential> cgraph(cgraph_obj);
  Int current_node = cgraph.bottom_node();
  Int top_node = cgraph.top_node();

  while (current_node != top_node) {
    Set<Int> neighbours = cgraph.out_adjacent_nodes(current_node);
    if (neighbours.size() > 1) return false;
    current_node = *(neighbours.begin());
  }

  return true;
}

// For a nested matroid, computes the maximal transversal presentation
Array<Set<Int>> nested_presentation(BigObject matroid)
{
  Int n = matroid.give("N_ELEMENTS");
  BigObject cyclic_flats_obj = matroid.give("LATTICE_OF_CYCLIC_FLATS");
  Lattice<BasicDecoration, Sequential> cyclic_flats(cyclic_flats_obj);
  Array<Set<Int>> ordered_faces(cyclic_flats.nodes());
  Array<Int> ordered_ranks(cyclic_flats.nodes());

  // Convert faces into ordered list
  ordered_faces[0] = cyclic_flats.face(cyclic_flats.bottom_node());
  ordered_ranks[0] = 0;
  Int current_index = 1;
  for (Int i = 0; i <= cyclic_flats.rank()-1; ++i) {
    const auto& n_of_dim = cyclic_flats.nodes_of_rank(i+1);
    if (n_of_dim.size() != 0) {
      ordered_faces[current_index] = cyclic_flats.face(*(n_of_dim.begin()));
      ordered_ranks[current_index] = i+1;
      ++current_index;
    }
  }

  // First set in presentation: Coloops as complements of largest cyclic flat
  Set<Int> coloops = sequence(0,n) - cyclic_flats.face(cyclic_flats.top_node());
  Int total_rank = coloops.size() + ordered_ranks[ ordered_ranks.size()-1];
  Array<Set<Int>> result(total_rank);
  Int presentation_index = 0;
  for (Int c = 0; c < coloops.size(); c++, ++presentation_index) {
    result[c] = coloops;
  }

  // Now move backwards in the list of flats and take complements
  for (Int j = ordered_faces.size()-2; j >= 0; --j) {
    Set<Int> complement = sequence(0, n) - ordered_faces[j];
    Int occ = ordered_ranks[j+1] - ordered_ranks[j];
    for (Int k = 0; k < occ; ++k, ++presentation_index) {
      result[presentation_index] = complement;
    }
  }

  return result;
}

UserFunction4perl("# @category Advanced properties"
                  "# Checks whether a matroid is nested, i.e. its lattice of cyclic flats is a chain."
                  "# @param Matroid M"
                  "# @return Bool Whether M is nested.",
                  &is_nested, "is_nested_matroid(Matroid)");

Function4perl(&nested_presentation, "nested_presentation(Matroid)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
