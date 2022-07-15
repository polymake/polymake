/* Copyright (c) 1997-2022
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

#include "polymake/client.h"
#include "polymake/Polynomial.h"

namespace polymake { namespace polytope {

BigObject hypersimplex(Int d, Int k, OptionSet options);

UniPolynomial<Rational, Int> ehrhart_polynomial_hypersimplex(Int k, Int d);

UniPolynomial<Rational, Int> ehrhart_polynomial_minimal_matroid(Int r, Int n);

UniPolynomial<Rational, Int> ehrhart_polynomial_panhandle_matroid(Int r, Int n, Int s);

UniPolynomial<Rational, Int> ehrhart_polynomial_cuspidal_matroid(Int r, Int n, Int s, Int k);

UniPolynomial<Rational, Int>  ehrhart_polynomial_product_simplicies(Int a, Int b);
} }


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
