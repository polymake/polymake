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
#include "polymake/Vector.h"
#include "polymake/Matrix.h"
#include "polymake/Set.h"
#include "polymake/Map.h"
#include "polymake/Array.h"
#include "polymake/list"
#include "polymake/TropicalNumber.h"
#include <string>

namespace polymake { namespace tropical {


      //FIXME: this client should be remodeled with support, vector difference, covector computation
      // Shouldn't the output be a set of vectors (matrix)?

	/*
	 * @brief Computes the sectors of the dual hyperplane centered at u that contain z.
	 * @return Set<Int>, subset of [0,... number of coordinates of u -1]
	 */
	template <typename Addition, typename Scalar, typename VectorTop>
		Set<int> containing_sectors(const GenericVector<VectorTop, TropicalNumber<Addition,Scalar> > &u, const GenericVector<VectorTop, TropicalNumber<Addition,Scalar> > &z) {
			Set<int> u_nonzero_entries = indices( attach_selector(u.top(), operations::non_zero()));
			Set<int> z_nonzero_entries = indices( attach_selector(z.top(), operations::non_zero()));
			Set<int> neither_zero = u_nonzero_entries * z_nonzero_entries;
			//If an entry in z is tropically zero, but finite in u, z lies in the corresponding sector.
			//In that case these are all the sectors
			Set<int> sectors = u_nonzero_entries - z_nonzero_entries;
			if(sectors.size() > 0) return sectors;
			//Compute the maximum of u_i - z_i over all pairwise nonzero entries.	
			TropicalNumber<Addition,Scalar> achieved = TropicalNumber<Addition,Scalar>::zero();
			Map<int, TropicalNumber<Addition,Scalar> > entry_diffs;
			for(Entire<Set<int> >::iterator zentry = entire(neither_zero); !zentry.at_end(); zentry++) {
				TropicalNumber<Addition,Scalar> ediff = (u.top()[*zentry] / z.top()[*zentry]);
				entry_diffs[*zentry] = ediff;
				achieved += ediff;
			}
			//We can only have sectors for entries in u that are non-tropically-zero
			for(Entire<Set<int> >::iterator uentry = entire(neither_zero); !uentry.at_end(); uentry++) {
				if(achieved == entry_diffs[*uentry]) sectors += (*uentry);
			}
			return sectors;
		}

	/*
	 * @brief Computes the set of tropical vertices from a matrix of tropical points
	 * in canonical form.
	 * @return Set<Int> The subset of row indices corresponding to vertices.
	 */
	template <typename Addition, typename Scalar>
		void discard_non_vertices(perl::Object cone)
		{
			Matrix<TropicalNumber<Addition,Scalar> > V = cone.give("POINTS");
			const int n(V.rows()), d(V.cols());
			Set< Vector<TropicalNumber<Addition, Scalar> > > vertex_coords;
			Set<int> vertex_indices;
			const	Vector<TropicalNumber<Addition, Scalar> > zerovec = zero_vector<TropicalNumber<Addition, Scalar> >(d);

			for (int i=0; i<n; ++i) {
				if(V.row(i) == zerovec) continue;
				if (vertex_coords.contains(V.row(i))) continue; // notice that it is possible that a point arises more than once
				int no_of_nonzero = attach_selector(V.row(i), operations::non_zero()).size();
				Set<int> sectors;
				for (int j=0; sectors.size()<no_of_nonzero && j<n; ++j) {
					if (V.row(j)==V.row(i)) continue;
					sectors += containing_sectors(V.row(j), V.row(i));
				}
				if (sectors.size()<no_of_nonzero) {
					vertex_coords+=V.row(i);
					vertex_indices+=i;
				}
			}

			cone.take("VERTICES_IN_POINTS") << Array<int>(vertex_indices);
			cone.take("VERTICES") << V.minor(vertex_indices,All); 
		}

	FunctionTemplate4perl("containing_sectors<Addition,Scalar>(Vector<TropicalNumber<Addition,Scalar> >, Vector<TropicalNumber<Addition,Scalar> >)");

	FunctionTemplate4perl("discard_non_vertices<Addition,Scalar>(Polytope<Addition,Scalar>) : void");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
