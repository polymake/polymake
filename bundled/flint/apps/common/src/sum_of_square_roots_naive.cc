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

#include <algorithm>
#include "polymake/client.h"
#include "polymake/Rational.h"
#include "polymake/Array.h"
#include "polymake/Map.h"
#include "polymake/common/factorization.h"

namespace polymake { namespace common {

Map<Rational, Rational> sum_of_square_roots_naive(const Array<Rational>& a)
{
   // To avoid calling the extremely inefficient factoring algorithm many times, 
   // first sort the array. If it comes from the squared volumes of a triangulation,
   // there will be many equal values, so this makes sense.
   Array<Rational> sorted_a(a);
   std::sort(sorted_a.begin(), sorted_a.end());

   // Now start the summation, counting how many equal values there are in a row
   Map<Rational, Rational> coefficient_of_sqrt;
   auto a1 = entire(sorted_a);
   auto a2 = a1;
   while (!a2.at_end()) {
      Int multiplicity = 1; 
      ++a2;
      while (!a2.at_end() && *a2 == *a1) { // how many times does the entry *a1 repeat?
         ++multiplicity;
         ++a2;
      } 
      const std::pair<Integer, Integer> // a2 could be at_end(), so use a1
         ir_sqrt_num = pm::flint::factor_out_squares(numerator(*a1)), 
         ir_sqrt_den = pm::flint::factor_out_squares(denominator(*a1));
      coefficient_of_sqrt[Rational(ir_sqrt_num.first, ir_sqrt_den.first)] += 
         multiplicity * Rational(ir_sqrt_num.second, ir_sqrt_den.second);
      a1 = a2; // could both be at_end(); that's ok
   }
   return coefficient_of_sqrt;
}

UserFunction4perl("# @category Arithmetic"
		  "# Make a naive attempt to sum the square roots of the entries of the input array."
		  "# @param Array<Rational> input_array a list of rational numbers (other coefficents are not implemented)."
		  "# @return Map<Rational, Rational> a map collecting the coefficients of roots encountered in the sum."
		  "# @example To obtain sqrt{3/4} + sqrt{245}, type"
                  "# > print sum_of_square_roots_naive(new Array<Rational>([3/4, 245]));"
                  "# | {(3 1/2) (5 7)}"
                  "# This output represents sqrt{3}/2 + 7 sqrt{5}." 
		  "# If you are not satisfied with the result, please use a symbolic algebra package.",
		  &sum_of_square_roots_naive, "sum_of_square_roots_naive(Array<Rational>)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:

