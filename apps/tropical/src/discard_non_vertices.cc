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
#include "polymake/Vector.h"
#include "polymake/Matrix.h"
#include "polymake/Set.h"
#include "polymake/Map.h"
#include "polymake/Array.h"
#include "polymake/list"
#include "polymake/TropicalNumber.h"
#include <string>

namespace polymake { namespace tropical {

// FIXME: this client should be remodeled with support, vector difference, covector computation
// Shouldn't the output be a set of vectors (matrix)?

/*
 * @brief Computes the sectors of the dual hyperplane centered at u that contain z.
 * @return Set<Int>, subset of [0,... number of coordinates of u -1]
 */
template <typename Addition, typename Scalar, typename VectorTop>
Set<Int> containing_sectors(const GenericVector<VectorTop, TropicalNumber<Addition, Scalar>>& u,
                            const GenericVector<VectorTop, TropicalNumber<Addition, Scalar>>& z)
{
   Set<Int> u_nonzero_entries = indices(attach_selector(u.top(), operations::non_zero()));
   Set<Int> z_nonzero_entries = indices(attach_selector(z.top(), operations::non_zero()));
   Set<Int> neither_zero = u_nonzero_entries * z_nonzero_entries;
   // If an entry in z is tropically zero, but finite in u, z lies in the corresponding sector.
   // In that case these are all the sectors
   Set<Int> sectors = u_nonzero_entries - z_nonzero_entries;
   if (!sectors.empty()) return sectors;
   // Compute the maximum of u_i - z_i over all pairwise nonzero entries.
   TropicalNumber<Addition, Scalar> achieved = TropicalNumber<Addition,Scalar>::zero();
   Map<Int, TropicalNumber<Addition, Scalar>> entry_diffs;
   for (auto zentry = entire(neither_zero); !zentry.at_end(); ++zentry) {
      TropicalNumber<Addition, Scalar> ediff = (u.top()[*zentry] / z.top()[*zentry]);
      entry_diffs[*zentry] = ediff;
      achieved += ediff;
   }
   // We can only have sectors for entries in u that are non-tropically-zero
   for (auto uentry = entire(neither_zero); !uentry.at_end(); ++uentry) {
      if (achieved == entry_diffs[*uentry]) sectors += *uentry;
   }
   return sectors;
}

/*
 * @brief Computes the set of tropical vertices from a matrix of tropical points
 * in canonical form.
 * @return Set<Int> The subset of row indices corresponding to vertices.
 */
template <typename Addition, typename Scalar>
void discard_non_vertices(BigObject cone)
{
   Matrix<TropicalNumber<Addition, Scalar>> V = cone.give("POINTS");
   const Int n = V.rows();
   Set<Vector<TropicalNumber<Addition, Scalar>>> vertex_coords;
   Set<Int> vertex_indices;

   for (Int i = 0; i < n; ++i) {
      if (is_zero(V.row(i))) continue;
      if (vertex_coords.contains(V.row(i))) continue; // notice that it is possible that a point arises more than once
      Int no_of_nonzero = attach_selector(V.row(i), operations::non_zero()).size();
      Set<Int> sectors;
      for (Int j = 0; sectors.size()<no_of_nonzero && j<n; ++j) {
         if (V.row(j)==V.row(i)) continue;
         sectors += containing_sectors(V.row(j), V.row(i));
      }
      if (sectors.size() < no_of_nonzero) {
         vertex_coords+=V.row(i);
         vertex_indices+=i;
      }
   }

   cone.take("VERTICES_IN_POINTS") << Array<Int>(vertex_indices);
   cone.take("VERTICES") << V.minor(vertex_indices,All);
}

FunctionTemplate4perl("containing_sectors<Addition,Scalar>(Vector<TropicalNumber<Addition,Scalar> >, Vector<TropicalNumber<Addition,Scalar> >)");

FunctionTemplate4perl("discard_non_vertices<Addition,Scalar>(Polytope<Addition,Scalar>)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
