/* Copyright (c) 1997-2019
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
#include "polymake/Set.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Map.h"
#include "polymake/graph/Lattice.h"
#include "polymake/graph/BasicLatticeTypes.h"

namespace polymake { namespace matroid {

using namespace graph;
using namespace graph::lattice;

// Computes the union of all circuits that are contained in flat
Set<int> cyclic_part_of_flat(const Set<int>& flat, const Array<Set<int>>& circuits)
{
  Set<int> result;
  for (auto c = entire(circuits); !c.at_end(); ++c) {
    if ((flat * (*c)).size() == c->size()) result += *c;
  }
  return result;
}

// Given a list of indices of sets in an incidence matrix, returns the
// list of indices representing the maximal sets among them (i.e. not contained in
// another set in the list)
Set<int> reduce_to_maximal_faces(const NodeMap<Directed, BasicDecoration>& decor, const Set<int>& indices)
{
  Set<int> bad_indices;
  for (auto i1 = entire(indices); !i1.at_end(); ++i1) {
    Set<int> remaining = indices - bad_indices - *i1;
    for (auto i2 = entire(remaining); !i2.at_end(); ++i2) {
      if (incl(decor[*i1].face, decor[*i2].face) <= 0) {
        bad_indices += (*i1);
        break;
      }
    }
  }
  return indices - bad_indices;
}

// NOTE Lattice of cyclic flats has to be built "by hand", since we cannot describe it
// via a closure operator

perl::Object lattice_of_cyclic_flats(perl::Object matroid)
{
  int n_elements = matroid.give("N_ELEMENTS");
  Array<Set<int> > circuits = matroid.give("CIRCUITS");
  perl::Object flats = matroid.give("LATTICE_OF_FLATS");
  Graph<Directed> adjacency = flats.give("ADJACENCY");
  NodeMap<Directed, BasicDecoration> flat_decor = flats.give("DECORATION");
  int rank = matroid.give("RANK");
  Set<int> coloops_complement = accumulate ( circuits, operations::add());
  int n_coloops = n_elements - coloops_complement.size();

  Vector<Set<int>> cyclic_faces;
  // Node indices of cyclic flats in the new lattice
  Map<Set<int>, int> cyclic_face_indices;
  // Map each node index of the old lattice to the node index of its
  // cyclic part in the new lattice
  Map<int, int> cyclic_part;

  Lattice<BasicDecoration, Sequential> lattice;

  // The bottom flat is always a cyclic flat
  int old_bottom_index =  flats.give("BOTTOM_NODE");
  Set<int> bottom_node = flat_decor[old_bottom_index].face;
  int bottom_index = lattice.add_node(BasicDecoration(bottom_node, 0));
  cyclic_face_indices[bottom_node] = bottom_index;
  cyclic_part[old_bottom_index] = bottom_index;
  if (rank-n_coloops > 0) {
    for (int r = 1; r <= rank-n_coloops; ++r) {
      Set<int> current_nodes = flats.call_method("nodes_of_rank", r);
      for (auto cn = entire(current_nodes); !cn.at_end(); ++cn) {
        Set<int> cn_face = flat_decor[*cn].face;
        Set<int> cpart = cyclic_part_of_flat(cn_face, circuits);
        if (cpart == cn_face) {
          int cn_index = lattice.add_node(BasicDecoration(cn_face, r));
          // Iterate over all nodes under it
          Set<int> nodes_below;
          for (auto in_e = entire(adjacency.in_edges(*cn)); !in_e.at_end(); ++in_e) {
            nodes_below += cyclic_part[in_e.from_node()];
          }
          nodes_below = reduce_to_maximal_faces( lattice.decoration(), nodes_below);
          for (auto nb = entire(nodes_below); !nb.at_end(); ++nb) {
            lattice.add_edge( *(nb), cn_index);
          }
          cyclic_face_indices[cn_face] = cn_index;
          cyclic_part[(*cn)] = cn_index;
        } else {
          cyclic_part[*cn] = cyclic_face_indices[cpart];
        }
      }
    }
  }

  return static_cast<perl::Object>(lattice);
}

Function4perl(&lattice_of_cyclic_flats,"lattice_of_cyclic_flats(Matroid)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
