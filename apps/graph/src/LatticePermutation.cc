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
#include "polymake/graph/Lattice.h"
#include "polymake/graph/Decoration.h"
#include "polymake/graph/LatticePermutation.h"

namespace polymake { namespace graph {

// This computes a nodes permutation from two Lattices, given a permutation to apply to the
// face decoration.
template <typename Decoration, typename SeqType, typename Permutation>
Permutation find_lattice_permutation(perl::Object hd_obj, perl::Object perm_hd_obj, const Permutation& perm)
{
  Lattice<Decoration, SeqType> hd(hd_obj);
  Lattice<Decoration, SeqType> perm_hd(perm_hd_obj);
  // Begin by applying the permutation to the decoration of the hd_obj
  if (perm.size())
    hd.permute_faces(perm);

  Permutation nodes_perm(hd.nodes());

  int bottom_rank = hd.rank(hd.bottom_node());
  int top_rank = hd.rank(hd.top_node());

  using nodes_list = typename Lattice<Decoration, SeqType>::nodes_of_rank_type;

  // Compute permutations for each rank
  for (int r = bottom_rank; r <= top_rank; ++r) {
    const nodes_list& level_nodes = hd.nodes_of_rank(r);
    const Array<int> perm_level_nodes(perm_hd.nodes_of_rank(r));
    const Array<Set<int> > level_faces(level_nodes.size(),
            entire( attach_member_accessor( select(hd.decoration(), level_nodes),
                                                  ptr2type<Decoration, Set<int>, &Decoration::face>())));
    const Array<Set<int> > perm_level_faces(perm_level_nodes.size(),
            entire( attach_member_accessor( select(perm_hd.decoration(), perm_level_nodes),
                                                  ptr2type<Decoration, Set<int>, &Decoration::face>())));
    Array<int> level_perm = find_permutation( perm_level_faces, level_faces);
    copy_range(entire(permuted(perm_level_nodes, level_perm)), select(nodes_perm, level_nodes).begin());
  }

  return nodes_perm;
}

UserFunctionTemplate4perl("# @category Combinatorics"
                          "# This takes two lattices and checks whether they are isomorphic, possibly after applying"
                          "# a permutation to the faces. This function only compares faces and ranks of nodes to determine"
                          "# isomorphism"
                          "# @param Lattice L1 A lattice"
                          "# @param Lattice L2 Another lattice, having the same decoration and sequential type"
                          "# @param Permutation permutation A permutation to be applied to the faces. If empty, "
                          "# the identity permutation is chosen"
                          "# @return Permutation A permutation on the nodes of the graph, if the lattices are isomorphic."
                          "# Otherwise an exeption is thrown.",
                          "find_lattice_permutation<Decoration, SeqType, Permutation>(Lattice<Decoration, SeqType>, Lattice<Decoration,SeqType>, Permutation)");
} }
