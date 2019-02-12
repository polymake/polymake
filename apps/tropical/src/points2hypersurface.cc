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
#include "polymake/TropicalNumber.h"
#include "polymake/tropical/dual_addition_version.h"

namespace polymake { namespace tropical {

template <typename Addition>
perl::Object points2hypersurface(const Matrix<TropicalNumber<Addition>>& points)
{
   using TDualNumber = TropicalNumber<typename Addition::dual> ;

   const int d(points.cols());
   const Matrix<TDualNumber> dual_points = dual_addition_version(points, 1);
                        
   Polynomial<TDualNumber> hyperpoly(TDualNumber::one(), d);

   for (auto pt = entire(rows(dual_points)); !pt.at_end(); pt++) {
      hyperpoly *= Polynomial<TDualNumber>(*pt, unit_matrix<int>(d));
   }

   perl::Object result("Hypersurface", mlist<typename Addition::dual>());
   result.take("POLYNOMIAL") << hyperpoly;

   return result;
}

UserFunctionTemplate4perl("# @category Producing a tropical hypersurface"
                          "# Constructs a tropical hypersurface defined by the linear"
                          "# hyperplanes associated to the given points."
                          "# Min-tropical points give rise to Max-tropical linear forms,"
                          "# and vice versa, and this method produces the hypersurface"
                          "# associated to the (tropical) product of these linear forms,"
                          "# that is, the union of the respective associated hyperplanes."
                          "# @param Matrix<TropicalNumber<Addition>> points"
                          "# @return Hypersurface"
                          "# @example This produces the union of two (generic) Max-hyperplanes,"
                          "# and assigns it to $H."
                          "# > $points = new Matrix<TropicalNumber<Min>>([[0,1,0],[0,0,1]]);"
                          "# > $H = points2hypersurface($points);",
                          "points2hypersurface<Addition>(Matrix<TropicalNumber<Addition>>)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
