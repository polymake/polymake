/*
	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 51 Franklin Street, Fifth Floor,
	Boston, MA  02110-1301, USA.

	---
	Copyright (C) 2011 - 2015, Simon Hampe <simon.hampe@googlemail.com>

	---
	Copyright (c) 2016-2020
	Ewgenij Gawrilow, Michael Joswig, and the polymake team
	Technische Universität Berlin, Germany
	https://polymake.org

	Contains functions to finding all points reachable from a standard direction in a cubic.
	*/

#ifndef POLYMAKE_ATINT_LINES_IN_CUBIC_REACHABLE
#define POLYMAKE_ATINT_LINES_IN_CUBIC_REACHABLE

#include "polymake/client.h"
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/TropicalNumber.h"
#include "polymake/Polynomial.h"

namespace polymake { namespace tropical {

/**
 * This contains the result of reachablePoints(...):
 * - The rays of the complex (non-tropically homogeneous)
 * - The maximal two-dimensional cells in terms of the rays
 * - The maximal one-dimensional cells in terms of the rays
 */
struct ReachableResult {
  Matrix<Rational> rays;
  IncidenceMatrix<> cells;
  IncidenceMatrix<> edges;
};

/**
   @brief Computes whether in a list of values the maximum is attained at least twice.
   @param Vector<Rational> values A list of values
   @return True, if the maximum is attained at least twice, false otherwise
*/
bool maximumAttainedTwice(const Vector<Rational>& values);

/**
   @brief This takes a cubic surface defined by a tropical polynomial f and a direction index in 0,1,2,3 and computes the set of all points p such that the line from p in the direction of e_0,-e1,..,-e3 lies in X.
   @param Polynomial<TropicalNumber<Max> > f A tropical polynomial of degree 3
   @param Cycle<Addition> X The divisor of f (in R^3)
   @param Int direction Lies in 0,1,2,3 and means we consider the direction e_0 = (1,1,1) or -e_i for i > 0
   (respectively the inverse for min).
   @return ReachableResult
*/
ReachableResult reachablePoints(const Polynomial<TropicalNumber<Max>>& f, BigObject X, Int direction); 

} }

#endif
