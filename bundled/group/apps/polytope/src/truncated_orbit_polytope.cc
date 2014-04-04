/* Copyright (c) 1997-2014
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
#include "polymake/group/permlib.h"
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/ListMatrix.h"
#include "polymake/Set.h"
#include "polymake/polytope/separating_hyperplane.h"
#include "polymake/polytope/sympol_interface.h"
#include "polymake/common/lattice_tools.h"

namespace polymake { namespace polytope {
namespace {

  perl::Object truncated_orbit_polytope(const Vector<Rational>& v, perl::Object group, Rational eps)
  {
    using namespace polymake::group;
    using namespace polymake::polytope::sympol_interface;

    const Matrix<Rational> gen_point = vector2row(v);
    
    typedef std::pair< ListMatrix< Vector<Rational> > , Array< Set<int> > > OrbitPair;
    OrbitPair gen_point_orbit = orbits_coord_action_complete_sub(group, gen_point);
    
    Array< Array<int> > gens=group.give("GENERATORS");
    PermlibGroup permlib_group(gens);

    Matrix<Rational> facets;
    Matrix<Rational> equations;
    // compute symmetry group of polytope, which is different because it acts on the vertex indices instead of coordinates
    PermlibGroup polytope_group = sympol_wrapper::compute_linear_symmetries(gen_point_orbit.first, Matrix<Rational>());
    // SymPol computation with one level of IDM (all vertices are in the same orbit) and one more level of ADM (exploit remaining symmetry)
    const bool res = sympol_wrapper::computeFacets(gen_point_orbit.first, Matrix<Rational>(), polytope_group, sympol_interface::lrs, 1, 2, true, facets, equations);
    if (!res) {
      throw std::runtime_error("truncated_orbit_polytope: sympol was not able to compute the facets...");
    }

    bool answer = false;
    Vector<Rational> sep_hyperplane;
    Set<int> vertex_indices;
    int i = 0;
    for (Entire<Rows<ListMatrix<Vector<Rational > > > >::const_iterator it = entire(rows(gen_point_orbit.first)); !it.at_end(); ++it) {
      if (*it != v) 
        vertex_indices += i;
      ++i;
    }
    is_vertex_sub(v, gen_point_orbit.first.minor(vertex_indices, All), answer, sep_hyperplane);

    // make sep_hyperplane an integral primitive vector
    Vector<Rational> normed_sep_hyp(common::primitive(sep_hyperplane));

    // move hyperplane so that v is separated by eps
    normed_sep_hyp[0] -= v *  normed_sep_hyp + eps;
    facets /=  normed_sep_hyp;
    
    perl::Object cp_polytope("SymmetricPolytope<Rational>");
    cp_polytope.take("GENERATING_GROUP") << correct_group_from_permlib_group(group, permlib_group);
    cp_polytope.take("GEN_INEQUALITIES") << facets;
    cp_polytope.take("GEN_EQUATIONS") << equations;
    return cp_polytope;
  }
}

UserFunction4perl("# @category Symmetry"
                  "# Constructs an orbit polytope of a given point //v// with respect to a given group //group//, "
                  "# in which all vertices are cut off by hyperplanes in distance //eps// "
                  "# @param Vector v point of which orbit polytope is to be constructed "
                  "# @param group::GroupOfPolytope group group for which orbit polytope is to be constructed"
                  "# @param Rational eps scaled distance by which the vertices of the orbit polytope are to be cut off"
                  "# @return SymmetricPolytope the truncated orbit polytope",
                  &truncated_orbit_polytope, "truncated_orbit_polytope(Vector, group::GroupOfPolytope, $)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:2
// indent-tabs-mode:nil
// End:
