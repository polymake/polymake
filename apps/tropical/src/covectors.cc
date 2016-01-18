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
#include "polymake/Set.h"
#include "polymake/Graph.h"
#include "polymake/Array.h"
#include "polymake/Matrix.h"
#include "polymake/linalg.h"
#include "polymake/TropicalNumber.h"
#include "polymake/tropical/covectors.h"

namespace polymake { namespace tropical {

	Matrix<int> coarse_covector_from_fine( const Array<IncidenceMatrix<> > &cov) {
		if(cov.size() == 0) return Matrix<int>();
		Matrix<int> result(cov.size(), cov[0].rows());
		int c_index = 0;
		for(Entire<Array<IncidenceMatrix<> > >::const_iterator c = entire(cov); !c.at_end(); c++) {

			for(Entire< Rows< IncidenceMatrix<> > >::const_iterator row_it = entire( rows(*c));
					!row_it.at_end(); row_it++) {
				result(c_index, row_it.index()) = (*row_it).size();
			}

			c_index++;
		}
		return result;
	}

	template <typename Addition, typename Scalar>
		Matrix<int> coarse_covectors( const Matrix<TropicalNumber<Addition,Scalar> > &points,
				const Matrix<TropicalNumber<Addition,Scalar> > &generators) {

			return coarse_covector_from_fine(covectors(points,generators));
		}

	template <typename Addition, typename Scalar>
		Matrix<int> coarse_covectors_of_scalar_vertices( const Matrix<Scalar> &points,
								const Matrix<TropicalNumber<Addition,Scalar> > &generators) {
			return coarse_covector_from_fine(covectors_of_scalar_vertices(points,generators));
		}


	UserFunctionTemplate4perl("# @category Tropical covector decomposition"
			"# This computes the (fine) covector of a list of points relative to a list of"
			"# generators."
			"# @param Matrix<TropicalNumber<Addition,Scalar> > points"
			"# @param Matrix<TropicalNumber<Addition,Scalar> > generators"
			"# @return Array<IncidenceMatrix>. Each IncidenceMatrix corresponds to a point."
			"# Rows of a matrix correspond to coordinates and columns"
			"# to generators. Each row indicates, which generators contain the point in the"
			"# sector corresponding to the coordinate.",
			"covectors<Addition,Scalar>(Matrix<TropicalNumber<Addition,Scalar> >, Matrix<TropicalNumber<Addition,Scalar> >)");


	UserFunctionTemplate4perl("# @category Tropical covector decomposition"
			"# This computes the (fine) covector of a list of points relative to a list of"
			"# generators."
			"# The points are scalar points and they are supposed to be normalized in the following sense:"
			"# - All bounded vertices have a leading 1"
			"# - All unbounded vertices have a leading 0 and all nonzero entries are either +1 or -1."
			"# (but not both)"
			"# Furthermore, the points make up a polyhedral complex - in particular, every maximal cell "
			"# has a bounded vertex."
			"# For the bounded vertices, covectors are computed as usual. For unbounded vertices, the"
			"# nonzero entries are replaced by tropical zero, the complementary entries"
			"# are copied from a bounded vertex sharing a cell and then the covector is computed."
			"# @param Matrix<Scalar> points"
			"# @param IncidenceMatrix maximal_cells"
			"# @param Matrix<TropicalNumber<Addition,Scalar> > generators"
			"# @return Array<IncidenceMatrix>. Each IncidenceMatrix corresponds to a point."
			"# Rows of a matrix correspond to coordinates and columns"
			"# to generators. Each row indicates, which generators contain the point in the"
			"# sector corresponding to the coordinate.",
			"covectors_of_scalar_vertices<Addition,Scalar>(Matrix<Scalar>,Matrix<TropicalNumber<Addition,Scalar> >)");

	UserFunctionTemplate4perl("# @category Tropical covector decomposition"
			"# This computes the coarse covector of a list of points relative to a list of"
			"# generators."
			"# @param Matrix<TropicalNumber<Addition,Scalar> > points"
			"# @param Matrix<TropicalNumber<Addition,Scalar> > generators"
			"# @return Matrix<int>. Rows correspond to points, columns to coordinates. Each entry "
			"# encodes, how many generators contain a given point in a certain coordinate.",
			"coarse_covectors<Addition,Scalar>(Matrix<TropicalNumber<Addition,Scalar> >, Matrix<TropicalNumber<Addition,Scalar> >)");

	UserFunctionTemplate4perl("# @category Tropical covector decomposition"
		"# Computes the coarse covectors of a list of scalar points, as described in "
		"# [[covectors_of_scalar_vertices]]"
		"# @param Matrix<Scalar> points"
		"# @param IncidenceMatrix maximal_cells"
		"# @param Matrix<TropicalNumber<Addition,Scalar> > generators"
		"# @return Matrix<int>. Rows correspond to points, columns to coordinates. Each entry "
		"# encodes, how many generators contain a given point in a certain coordinate.",
		"coarse_covectors_of_scalar_vertices<Addition,Scalar>(Matrix<Scalar>,Matrix<TropicalNumber<Addition,Scalar> >)");


      FunctionTemplate4perl("artificial_ray_covector<Addition,Scalar>(Set<Int>, Matrix<TropicalNumber<Addition, Scalar> >)");



} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
