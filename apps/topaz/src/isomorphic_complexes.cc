/* Copyright (c) 1997-2020
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
#include "polymake/graph/compare.h"
#include "polymake/IncidenceMatrix.h"

namespace polymake { namespace topaz {

bool isomorphic(BigObject p1, BigObject p2)
{
   const IncidenceMatrix<> M1=p1.give("FACETS"), M2=p2.give("FACETS");
   return graph::isomorphic(M1, M2);
}

optional<std::pair<Array<Int>, Array<Int>>>
find_facet_vertex_permutations(BigObject p1, BigObject p2)
{
   const IncidenceMatrix<> M1=p1.give("FACETS"), M2=p2.give("FACETS");
   return graph::find_row_col_permutation(M1,M2);
}

UserFunction4perl("# @category Comparing\n"
                  "# Determine whether two given complexes are combinatorially isomorphic.\n"
                  "# The problem is reduced to graph isomorphism of the vertex-facet incidence graphs.\n"
                  "# @param SimplicialComplex complex1"
                  "# @param SimplicialComplex complex2"
                  "# @return Bool",
                  &isomorphic, "isomorphic(SimplicialComplex,SimplicialComplex)");

UserFunction4perl("# @category Comparing\n"
                  "# Find the permutations of facets and vertices which maps the first complex to the second one.\n"
                  "# The facet permutation is the first component of the return value.\n"
                  "# @param SimplicialComplex complex1"
                  "# @param SimplicialComplex complex2"
                  "# @return Pair<Array<Int>, Array<Int>> permutations of facet and vertex indexes, or undef when complexes are not isomorphic",
                  &find_facet_vertex_permutations, "find_facet_vertex_permutations(SimplicialComplex,SimplicialComplex)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
