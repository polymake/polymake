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

#include "polymake/client.h"
#include "polymake/Vector.h"
#include "polymake/TropicalNumber.h"

namespace polymake { namespace tropical {

template <typename Addition, typename Scalar, typename VectorTop>
Scalar norm(const GenericVector<VectorTop, TropicalNumber<Addition,Scalar> >& vec)
{
   const Scalar min(accumulate(vec.top(),operations::min()));
   const Scalar max(accumulate(vec.top(),operations::max()));
   try {
		return max-min;
	}
	catch(...) {
		//This goes wrong if vec is tropically zero everywhere
		throw std::runtime_error("The tropical norm is not defined for the tropical zero-vector");
	}
}
  
UserFunctionTemplate4perl("# @category Tropical operations"
                          "# The __tropical norm__ of a vector //v// in tropical projective space"
                          "# is the difference between the maximal and minimal coordinate"
                          "# in any coordinate representation of the vector."
                          "# @param Vector<TropicalNumber<Addition,Scalar>> v"
                          "# @return Scalar"
                          "# @example"
                          "# > $v = new Vector<TropicalNumber<Min>>([1,-2,3]);"
                          "# > print norm($v);"
                          "# | 5"
                          "# @example"
                          "# > $w = new Vector<TropicalNumber<Min>>([0,'inf',3]);"
                          "# > print norm($w);"
                          "# | inf",
                          "norm<Addition,Scalar>(Vector<TropicalNumber<Addition,Scalar>>)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
