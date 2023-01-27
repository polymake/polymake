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

#pragma once

#include "polymake/linalg.h"
#include "polymake/Vector.h"
#include "polymake/Matrix.h"
#include "polymake/Rational.h"
#include "polymake/hash_map"
#include "polymake/polytope/canonicalize.h"
#include "polymake/polytope/convex_hull.h"

namespace polymake { namespace fan {

namespace {

template <typename Scalar, typename TMatrix>
Set<Int>
indices_of(const GenericMatrix<TMatrix, Scalar>& rays,
           hash_map<Vector<Scalar>, Int>& index_of,
           Int& i)
{
   Set<Int> indices_of;
   for (auto rit = entire(rows(rays)); !rit.at_end(); ++rit) {
      Vector<Scalar> v(*rit);
      polytope::canonicalize_oriented(entire(v));
      if (!index_of.exists(v)) {
         index_of[v] = i++;
      }
      indices_of += index_of[v];
   }
   return indices_of;
}

} // end anonymous namespace


template <typename Scalar, typename TMatrix>
Matrix<Scalar>
rays_of_intersection(const GenericMatrix<TMatrix,Scalar>& V,
                     const Matrix<Scalar>& intersection_lineality,
                     const Matrix<Scalar>& H)
{
   BigObject C("Cone", mlist<Scalar>(), "INPUT_RAYS", Matrix<Scalar>(V), "INPUT_LINEALITY", intersection_lineality);
   const Matrix<Scalar> facets = C.give("FACETS");

   BigObject D("Cone", mlist<Scalar>(), "INEQUALITIES", facets, "EQUATIONS", H);
   Matrix<Scalar> rays = D.give("RAYS");

   project_to_orthogonal_complement(rays, intersection_lineality);
   for (auto rit = entire(rows(rays)); !rit.at_end(); ++rit)
      polytope::canonicalize_oriented(entire(*rit));
   return rays;
}

#if 0
template<typename TMatrix>
Matrix<Rational>
rays_of_intersection(const GenericMatrix<TMatrix, Rational>& V,
                     const Matrix<Rational>& intersection_lineality,
                     const Matrix<Rational>& H)
{
   const bool is_cone = true;
   const Matrix<Rational> facets = polytope::enumerate_facets(V, intersection_lineality, is_cone).first;
   Matrix<Rational> rays = polytope::enumerate_vertices(facets, H, is_cone).first;

   project_to_orthogonal_complement(rays, intersection_lineality);
   for (auto rit = entire(rows(rays)); !rit.at_end(); ++rit)
      polytope::canonicalize_oriented(entire(*rit));
   return rays;
}
#endif

} }


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
