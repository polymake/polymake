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
#include "polymake/TropicalNumber.h"
#include "polymake/tropical/dual_addition_version.h"

namespace polymake { namespace tropical {

	template <typename Addition>
		perl::Object points2hypersurface(const Matrix<TropicalNumber<Addition> >& points)
		{
			typedef TropicalNumber<typename Addition::dual>  TDualNumber;

			const int d(points.cols());
			const Matrix<TDualNumber> dual_points = 
				dual_addition_version(points,1);
			
			Ring<TDualNumber> r(d);
			Polynomial<TDualNumber > hyperpoly(TDualNumber::one(), r);

			for(typename Entire<Rows<Matrix<TDualNumber> > >::const_iterator pt = entire(rows(dual_points));
					!pt.at_end(); pt++) {
				Matrix<int> monoms = unit_matrix<int>(d);
				hyperpoly *= Polynomial<TDualNumber >(monoms, *pt, r);
			}

			perl::Object result(perl::ObjectType::construct<typename Addition::dual>("Hypersurface"));
				result.take("POLYNOMIAL") << hyperpoly;

			return result;
		
		}

	UserFunctionTemplate4perl("# @category Producing a tropical hypersurface"
			"# Constructs a tropical hypersurface defined by the linear"
			"# hypersurfaces associated to the points."
			"# If the points are min-tropical points then the output is a"
			"# max-tropical hypersurface, and conversely."
			"# @param Matrix<TropicalNumber<Addition> > points"
			"# @return Hypersurface",
			"points2hypersurface<Addition>(Matrix<TropicalNumber<Addition> >)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
