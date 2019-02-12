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
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/ListMatrix.h"
#include "polymake/hash_set"
#include "polymake/Set.h"
#include "polymake/polytope/separating_hyperplane.h"
#include "polymake/common/lattice_tools.h"

namespace polymake { namespace polytope {

template<typename Scalar>
perl::Object truncated_orbit_polytope(perl::Object p, Scalar eps)
{
   const Matrix<Scalar> 
      vertices = p.give("VERTICES"),
      linear_span = p.give("AFFINE_HULL");

   ListMatrix<Vector<Scalar>> inequalities_reps = p.give("GROUP.REPRESENTATIVE_FACETS");

   const Array<hash_set<int>> vertex_orbits = p.give("GROUP.VERTICES_ACTION.ORBITS");
   const Array<Array<int>> action_gens = p.give("GROUP.COORDINATE_ACTION.GENERATORS | GROUP.COORDINATE_ACTION.STRONG_GENERATORS");

   for (const auto& orbit : vertex_orbits) {
      // find an integral primitive separating hyperplane
      const int i0 (*(orbit.begin()));
      Vector<Scalar> h(common::primitive(separating_hyperplane(vertices[i0], vertices.minor(~scalar2set(i0), All))));
      // move hyperplane so that v is separated by eps
      h[0] -= vertices[i0] * h + eps;
      inequalities_reps /=  h;
   }

   perl::Object a("group::PermutationAction");
   a.take("GENERATORS") << action_gens;
   a.take("INEQUALITIES_GENERATORS") << inequalities_reps;

   perl::Object q("Polytope", mlist<Scalar>());
   q.take("AFFINE_HULL") << linear_span;
   q.take("GROUP.COORDINATE_ACTION") << a;

   return q;
}


UserFunctionTemplate4perl("# @category Symmetry"
                          "# Gives an implicit representation of the all-vertex truncation of an orbit polytope //P//,"
                          "# in which all vertices are cut off by hyperplanes at distance //eps//."
                          "# The input polytope //P// must have a __GROUP.COORDINATE_ACTION__."
                          "# The output is a polytope with a __GROUP.COORDINATE_ACTION__ equipped with"
                          "# __INEQUALITIES_GENERATORS__."
                          "# @param Polytope P the input polytope"
                          "# @param Scalar eps scaled distance by which the vertices of the orbit polytope are to be cut off"
                          "# @return Polytope the truncated orbit polytope",
                          "truncated_orbit_polytope<Scalar>(Polytope<type_upgrade<Scalar>>, type_upgrade<Scalar>)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
