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
#include "polymake/tropical/dual_addition_version.h"

namespace polymake { namespace tropical {
	
	//Note: This is not in dual_addition_version.h, as the compiler gets confused 
	//with all the overloaded functions when dealing with big objects.

	template <typename Addition, typename Scalar>
		perl::Object dual_addition_version_cone(perl::Object cone, bool strong = true) {
			Matrix<TropicalNumber<Addition,Scalar> > points = cone.give("POINTS");

			typedef typename pm::concat_list<typename Addition::dual, Scalar>::type cone_type;
			perl::Object result(perl::ObjectType::construct<cone_type>("Polytope"));
				result.take("POINTS") << dual_addition_version(points,strong);

			return result;
		}

	//FIXME This should be a direct UserFunctionTemplate4perl as the others, but it seems to confuse
	// perl, as there is some conflict with the Cycle-version. Currently there is a wrapper function
	// in cone_properties.rules.

	FunctionTemplate4perl("dual_addition_version_cone<Addition,Scalar>(Polytope<Addition,Scalar>;$=1)");


}}



