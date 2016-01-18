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
#include "polymake/graph/compare.h"
#include "polymake/IncidenceMatrix.h"

namespace polymake { namespace topaz {

bool isomorphic(perl::Object p1, perl::Object p2)
{
   const IncidenceMatrix<> M1=p1.give("FACETS"), M2=p2.give("FACETS");
   return graph::isomorphic(M1, M2);
}

std::pair< Array<int>, Array<int> >
find_facet_vertex_permutations(perl::Object p1, perl::Object p2)
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
                  "# If the complexes are not isomorphic, an exception is thrown.\n"
                  "# @param SimplicialComplex complex1"
                  "# @param SimplicialComplex complex2"
                  "# @return Pair<Array<Int>, Array<int>>"
                  "# @example To print the vertex permutation that maps the 3-simplex to its mirror image, type this:"
                  "# > $p = find_facet_vertex_permutations(simplex(3),scale(simplex(3),-1));"
                  "# > print $p->first;"
                  "# | 3 2 1 0",
                  &find_facet_vertex_permutations, "find_facet_vertex_permutations(SimplicialComplex,SimplicialComplex)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
