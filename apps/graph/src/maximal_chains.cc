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
#include "polymake/Set.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/graph/Lattice.h"
#include "polymake/Graph.h"
#include "polymake/graph/maximal_chains.h"

namespace polymake { namespace graph {

template <typename Decoration, typename SeqType>
IncidenceMatrix<> maximal_chains_of_lattice(perl::Object face_lattice_obj, perl::OptionSet options)
{
  Lattice<Decoration, SeqType> lattice(face_lattice_obj);
  bool ignore_bottom_node = options["ignore_bottom_node"];
  bool ignore_top_node = options["ignore_top_node"];
  const Array<Set<int> > max_chains = maximal_chains( lattice, ignore_bottom_node, ignore_top_node);
  return IncidenceMatrix<>(max_chains);
}

template <typename Decoration, typename SeqType>
perl::Object lattice_of_chains(perl::Object lattice_obj)
{
  Lattice<Decoration, SeqType> lattice(lattice_obj);
  Array<Set<int> > max_chains = maximal_chains(lattice,false, false);
  perl::Object chains_complex("topaz::SimplicialComplex");
  chains_complex.take("FACETS") << max_chains;
  perl::Object chain_hasse = chains_complex.give("HASSE_DIAGRAM");
  return chain_hasse;
}

UserFunctionTemplate4perl("# @category Combinatorics"
                          "# For a given lattice, this computes the lattice of chains from bottom to top node."
                          "# The result always includes an artificial top node."
                          "# @param Lattice<Decoration> lattice"
                          "# @return Lattice<BasicDecoration> Faces are sets of nodes of elements in the original"
                          "# lattice forming a chain, ranks are lenghts of chains"
                          "# @example [application polytope] The following prints all faces with their ranks of the lattice of"
                          "# chains of the face lattice of the 0-simplex (a single point):"
                          "# > print lattice_of_chains(simplex(0)->HASSE_DIAGRAM)->DECORATION;"
                          "# | ({-1} 3)"
                          "# | ({0 1} 2)"
                          "# | ({0} 1)"
                          "# | ({1} 1)"
                          "# | ({} 0)",
                          "lattice_of_chains<Decoration, SeqType>(Lattice<Decoration, SeqType>)");

UserFunctionTemplate4perl("# @category Combinatorics"
                          "# Computes the set of maximal chains of a Lattice object."
                          "# @param Lattice F"
                          "# @option Bool ignore_bottom_node If true, the bottom node is not included in the chains. False by default"
                          "# @option Bool ignore_top_node If true, the top node is not included in the chains. False by default"
                          "# @return IncidenceMatrix Each row is a maximal chain, "
                          "# indices refer to nodes of the Lattice"
                          "# @example [application polytope] [prefer cdd] The following prints all maximal chains of the face lattice of the"
                          "# 1-simplex (an edge):"
                          "# > print maximal_chains_of_lattice(simplex(1)->HASSE_DIAGRAM);"
                          "# | {0 1 3}"
                          "# | {0 2 3}",
                          "maximal_chains_of_lattice<Decoration, SeqType>(Lattice<Decoration, SeqType>, {ignore_bottom_node=>0, ignore_top_node=>0})");
} }
