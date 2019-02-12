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
#include "polymake/Bitset.h"
#include "polymake/Array.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Set.h"
#include "polymake/PowerSet.h"
#include "polymake/Graph.h"
#include "polymake/graph/Lattice.h"
#include "polymake/graph/Decoration.h"

namespace polymake { namespace matroid {

using graph::Lattice;
using graph::lattice::Sequential;
using graph::lattice::BasicDecoration;

// Returns the set of all node indices such that the corresponding face strictly contains
// the given set
// If the face is actually in the lattice, it returns its node index as a second value
// (which is -1 otherwise).
std::pair<Set<int>, int> faces_above(const Lattice<BasicDecoration, Sequential>& G, const Set<int>& face)
{
  Set<int> result;
  int face_index = -1;

  for (auto d_it = entire<indexed>(G.decoration()); !d_it.at_end(); ++d_it) {
    int incl_result = incl(face, d_it->face);
    if (incl_result < 0) {
      result += d_it.index();
    } else if (incl_result == 0) {
      face_index = d_it.index();
    }
  }

  return std::make_pair(result, face_index);
}


// Computes whether a matroid is transversal and if so, what a presentation is
// Based on the algorithm in Bonin: An introduction to transversal matroids
// arXiv: http://home.gwu.edu/~jbonin/TransversalNotes.pdf
// Note that it is sufficient to check that the multiplicity of a flat is >= 0,
// instead of checking this for all sets.
perl::ListReturn check_transversality(perl::Object matroid)
{
  const perl::Object flats_obj = matroid.give("LATTICE_OF_FLATS");
  const Lattice<BasicDecoration, Sequential> flats(flats_obj);
  const perl::Object cyclic_flats_obj = matroid.give("LATTICE_OF_CYCLIC_FLATS");
  const Lattice<BasicDecoration, Sequential> cyclic_flats(cyclic_flats_obj);
  int total_rank = matroid.give("RANK");
  const int n_elem = matroid.give("N_ELEMENTS");
  Set<int> loops = matroid.give("LOOPS");

  Map<int,int> multiplicity; // Maps cyclic flat indices to multiplicities

  perl::ListReturn r;

  for (int i = flats.rank(); i >= 0 ; --i) {
    for (const auto n : flats.nodes_of_rank(i)) {
      std::pair<Set<int>, int> parents = faces_above(cyclic_flats, flats.face(n));
      int mult_value = total_rank - i;
      for (const int p : parents.first) {
        mult_value -= multiplicity[p];
      }
      if (mult_value < 0) {
        r << false;
        return r;
      }
      if (parents.second != -1) {
        multiplicity[parents.second] = mult_value;
      }
    }
  }

  std::vector<Set<int>> potential_presentation;
  const auto total_set = sequence(0, n_elem);
  for (const auto& mp : multiplicity) {
    int this_mult = mp.second;
    if (this_mult > 0) {
      const Set<int> this_face = total_set - cyclic_flats.face(mp.first);
      for (int i=0; i < this_mult; ++i)
        potential_presentation.push_back(this_face);
    }
  }

  r << true << Array<Set<int>>(potential_presentation);

  return r;
}

UserFunction4perl("# @category Advanced properties"
                  "# Checks whether a matroid is transversal."
                  "# If so, returns one possible transversal presentation"
                  "# @param Matroid M"
                  "# @return List(Bool, Array<Set<Int> >)"
                  "# First a bool indicating whether M is transversal"
                  "# If this is true, the second entry is a transversal presentation"
                  "# @example Computes whether the uniform matroid of rank 3 on 4 elements is transversal."
                  "# > @a = check_transversality(uniform_matroid(3,4));"
                  "# > print $a[0];"
                  "# | true"
                  "# > print $a[1];"
                  "# | {0 1 2 3}"
                  "# | {0 1 2 3}"
                  "# | {0 1 2 3}",
                  &check_transversality, "check_transversality(Matroid)");

} }
