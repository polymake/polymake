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

#include <algorithm>
#include "polymake/client.h"
#include "polymake/Rational.h"
#include "polymake/Array.h"
#include "polymake/Map.h"
#include "polymake/common/primes.h"

namespace polymake { namespace common {

typedef long number_t;
typedef Map<number_t, number_t> ExponentMap;

ExponentMap naive_partial_prime_factorization(const Integer& n)
{
   Integer rest(n);
   ExponentMap exponent_of;
   number_t const* pptr = polymake_primes;
   int i=0;
   while (i < n_polymake_primes && rest > 1) {
      const Div<Integer> qr = div(rest, *pptr);
      if (qr.rem == 0) { 
         exponent_of[*pptr]++;
         rest = qr.quot;
      } else 
         ++pptr, ++i;
   }
   if (rest > 1) {
      exponent_of[rest.to_long()] = 1;
      cerr << "Warning: did not completely factorize the input. The result may simplify further."
           << endl;
   }
   return exponent_of;
}

std::pair<number_t, number_t> integer_and_radical_of_sqrt(const Integer &n)
{
   const ExponentMap exponent_of = naive_partial_prime_factorization(n);
   std::pair<number_t, number_t> ir_sqrt(1,1);
   for (Entire<ExponentMap>::const_iterator mit = entire(exponent_of); !mit.at_end(); ++mit) {
      number_t rest_exponent = mit->second;
      if (mit->second & 1) { // odd exponent
         ir_sqrt.second *= mit->first;
         rest_exponent--;
      }
      while (rest_exponent) {
         ir_sqrt.first *= mit->first;
         rest_exponent -= 2;
      }
   }
   return ir_sqrt;
}

Map<Rational, Rational> sum_of_square_roots_naive(const Array<Rational>& a)
{
   // To avoid calling the extremely inefficient factoring algorithm many times, 
   // first sort the array. If it comes from the squared volumes of a triangulation,
   // there will be many equal values, so this makes sense.
   Array<Rational> sorted_a(a);
   std::sort(sorted_a.begin(), sorted_a.end());

   // Now start the summation, counting how many equal values there are in a row
   Map<Rational, Rational> coefficient_of_sqrt;
   Entire<Array<Rational> >::const_iterator a1 = entire(sorted_a), a2 = a1;
   while (!a2.at_end()) {
      int multiplicity(1); 
      ++a2;
      while (!a2.at_end() && *a2 == *a1) { // how many times does the entry *a1 repeat?
         ++multiplicity;
         ++a2;
      } 
      const std::pair<number_t, number_t> // a2 could be at_end(), so use a1
         ir_sqrt_num = integer_and_radical_of_sqrt(numerator(*a1)), 
         ir_sqrt_den = integer_and_radical_of_sqrt(denominator(*a1));
      coefficient_of_sqrt[Rational(ir_sqrt_num.second, ir_sqrt_den.second)] += 
         multiplicity * Rational(ir_sqrt_num.first, ir_sqrt_den.first);
      a1 = a2; // could both be at_end(); that's ok
   }
   return coefficient_of_sqrt;
}

UserFunction4perl("# @category Arithmetic"
		  "# Make a naive attempy to sum the square roots of the entries"
		  "# of the input array."
		  "# @param Array<Rational> a list of rational numbers (other coefficents are not implemented)."
		  "# @return Map<Rational, Rational> coefficient_of_sqrt a map collecting the coefficients of various roots encountered in the sum."
		  "# For example, {(3 1/2),(5 7)} represents sqrt{3}/2 + 7 sqrt{5}." 
		  "# If the output is not satisfactory, please use a symbolic algebra package.",
		  &sum_of_square_roots_naive, "sum_of_square_roots_naive(Array<Rational>)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:

