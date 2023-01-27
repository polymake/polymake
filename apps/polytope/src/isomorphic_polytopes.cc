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
#include "polymake/graph/compare.h"
#include "polymake/IncidenceMatrix.h"

namespace polymake { namespace polytope {

bool isomorphic(BigObject p1, BigObject p2)
{
   const IncidenceMatrix<> M1=p1.give("RAYS_IN_FACETS"), M2=p2.give("RAYS_IN_FACETS");
   return graph::isomorphic(M1, M2);
}

bool is_self_dual(BigObject p)
{
   const IncidenceMatrix<> M=p.give("RAYS_IN_FACETS");
   return graph::isomorphic(M, T(M));
}

optional<std::pair<Array<Int>, Array<Int>>>
find_facet_vertex_permutations(BigObject p1, BigObject p2)
{
   const IncidenceMatrix<> M1 = p1.give("RAYS_IN_FACETS"), M2 = p2.give("RAYS_IN_FACETS");
   return graph::find_row_col_permutation(M1, M2);
}

UserFunction4perl("# @category Comparing"
                  "# Check whether the face lattices of two cones or polytopes are isomorphic."
                  "# The problem is reduced to graph isomorphism of the vertex-facet incidence graphs."
                  "# @param Cone P1 the first cone/polytope"
                  "# @param Cone P2 the second cone/polytope"
                  "# @return Bool 'true' if the face lattices are isomorphic, 'false' otherwise"
                  "# @example The following compares the standard 2-cube with a polygon generated as"
                  "# the convex hull of five points.  The return value is true since both polygons are"
                  "# quadrangles."
                  "# > $p = new Polytope(POINTS=>[[1,-1,-1],[1,1,-1],[1,-1,1],[1,1,1],[1,0,0]]);"
                  "# > print isomorphic(cube(2),$p);"
                  "# | true",
                  &isomorphic, "isomorphic(Cone,Cone)");

UserFunction4perl("# @category Comparing"
                  "# Find the permutations of facets and vertices which maps the cone or polyhedron //P1// to //P2//."
                  "# The facet permutation is the first component, the vertex permutation is the second component of the return value."
                  "# "
                  "# Only the combinatorial isomorphism is considered."
                  "# @param Cone P1 the first cone/polytope"
                  "# @param Cone P2 the second cone/polytope"
                  "# @return Pair<Array<Int>, Array<Int>> the facet and the vertex permutations, or undef if polytopes are not isomorphic"
                  "# @example [prefer cdd] [require bundled:cdd] To print the vertex permutation that maps the 3-simplex to its mirror image, type this:"
                  "# > $p = find_facet_vertex_permutations(simplex(3),scale(simplex(3),-1));"
                  "# > print $p->first;"
                  "# | 1 2 3 0",
                  &find_facet_vertex_permutations, "find_facet_vertex_permutations(Cone,Cone)");

Function4perl(&is_self_dual, "is_self_dual(Cone)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
