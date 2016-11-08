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
#include "polymake/Rational.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/Set.h"
#include "polymake/TropicalNumber.h"
#include "polymake/tropical/covectors.h"

namespace polymake { namespace tropical {

	template <typename Addition, typename Scalar>
		void compute_maximal_covectors(perl::Object cone) {
			Matrix<Rational> pseudovertices = cone.give("PSEUDOVERTICES");
			IncidenceMatrix<> maximal_cells = cone.give("MAXIMAL_COVECTOR_CELLS");
			Matrix<TropicalNumber<Addition,Scalar> > points = cone.give("POINTS");

			Matrix<Rational> interior_points(maximal_cells.rows(), pseudovertices.cols());
			int index=0;
			for(Entire<Rows<IncidenceMatrix<> > >::iterator r = entire(rows(maximal_cells)); 
					!r.at_end(); r++, index++) {
				interior_points.row(index) = accumulate( rows(pseudovertices.minor(*r,All)),operations::add()) 
														/ support(pseudovertices.minor(*r,All).col(0)).size(); 
			}
			cone.take("MAXIMAL_COVECTORS") << covectors_of_scalar_vertices(interior_points, points);
		}

	FunctionTemplate4perl("compute_maximal_covectors<Addition,Scalar>(Polytope<Addition,Scalar>) : void");

}}


