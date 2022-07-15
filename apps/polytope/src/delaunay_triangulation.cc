/* Copyright (c) 1997-2022
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
#include "polymake/IncidenceMatrix.h"
#include "polymake/Array.h"
#include "polymake/vector"
#include "polymake/Set.h"
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/linalg.h"

namespace polymake { namespace polytope {

template <typename E>
Array<Set<Int>> delaunay_triangulation(BigObject p)
{
   const IncidenceMatrix<> v_i_f = p.give("VERTICES_IN_FACETS");

   // delete the unbounded vertices and the facet at infinity
   const Set<Int> far_face = p.give("FAR_FACE");
   const IncidenceMatrix<> v_i_f_without_far_face=v_i_f.minor(sequence(0, v_i_f.rows()-1), ~far_face);

   const Int dim = p.call_method("DIM");
   const Matrix<E> points = p.give("FACETS");
   Matrix<E> sites = p.give("SITES");
   if (sites.cols()+2 == points.cols()) {
      // SITES is a vector configuration
      sites = ones_vector<E>() | sites;
   }

   std::vector<Set<Int>> triang;
   BigObjectType poly_type("Polytope", mlist<E>());

   for (auto sim=entire(cols(v_i_f_without_far_face)); !sim.at_end(); ++sim) {
      if (sim->size()==dim) {
         if (det(sites.minor(*sim, All)) != 0) triang.emplace_back(*sim);
      }
      else if (sim->size()>dim) { //we have to triangulate ourselves
         BigObject facet_poly(poly_type, "VERTICES", points.minor(*sim, All));
         const Array<Set<Int>> facet_triang = facet_poly.give("TRIANGULATION.FACETS");
         for (const Set<Int>& j : facet_triang) {
            const Set<Int> sim2(select(*sim, j));
            if (det(sites.minor(sim2, All)) != 0) triang.push_back(sim2);
         }
      }
   }
   return Array<Set<Int>>(triang);
}

UserFunctionTemplate4perl("# @category Triangulations, subdivisions and volume"
                          "# Compute the Delaunay triangulation of the given [[SITES]] of a VoronoiPolyhedron //V//. If the sites are"
                          "# not in general position, the non-triangular facets of the Delaunay subdivision are"
                          "# triangulated (by applying the beneath-beyond algorithm)."
                          "# @param VoronoiPolyhedron V"
                          "# @return Array<Set<Int>>"
                          "# @example [prefer cdd] [require bundled:cdd]"
                          "# > $VD = new VoronoiPolyhedron(SITES=>[[1,1,1],[1,0,1],[1,-1,1],[1,1,-1],[1,0,-1],[1,-1,-1]]);"
                          "# > $D = delaunay_triangulation($VD);"
                          "# > print $D;"
                          "# | {0 1 3}"
                          "# | {1 3 4}"
                          "# | {1 2 4}"
                          "# | {2 4 5}",
                          "delaunay_triangulation<Scalar>(VoronoiPolyhedron<Scalar>)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
