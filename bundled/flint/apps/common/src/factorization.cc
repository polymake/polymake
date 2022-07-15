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

#include "polymake/common/factorization.h"

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshadow"
#pragma clang diagnostic ignored "-Wconversion"
#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#endif

#include <flint/fmpz.h>

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

namespace pm {

namespace flint {

Integer fmpz_t_to_Integer(const fmpz_t f){
   mpz_t tmp;
   mpz_init(tmp);
   fmpz_get_mpz(tmp, f);
   Integer result(std::move(tmp));
   return result;
}


Map<Integer, Int> factor(const Integer& n){
   // Factors n into prime numbers
   fmpz_t flintn;
   fmpz_init(flintn);
   fmpz_set_mpz(flintn, n.get_rep());
   fmpz_factor_t flintFactorization;
   fmpz_factor_init(flintFactorization);
   fmpz_factor(flintFactorization, flintn);
   
   // Parse result
   slong i, len;
   len = flintFactorization->num;
   Map<Integer, Int> result;
   for (i = 0; i < len; i++){
      Integer key = fmpz_t_to_Integer(&(flintFactorization->p[i]));
      result[key] = flintFactorization->exp[i];
   }
   fmpz_clear(flintn);
   fmpz_factor_clear(flintFactorization);
   return result;
}


Integer expand(const Map<Integer, Int>& m){
   fmpz_factor_t fc;
   fmpz_factor_init(fc);
   fc->sign = 1;
   fmpz_t flint_n;
   for(const auto& p : m){
      fmpz_init(flint_n);
      fmpz_set_mpz(flint_n, p.first.get_rep());
      _fmpz_factor_append(fc, flint_n, p.second);
   }
   fmpz_init(flint_n);
   fmpz_factor_expand(flint_n,fc);
   Integer result = fmpz_t_to_Integer(flint_n);
   fmpz_clear(flint_n);
   fmpz_factor_clear(fc);
   return result;
}

  
TropicalNumber<Min> valuation(const Rational& x, const Integer& p) {
  if (x.is_zero()) {
    return TropicalNumber<Min>(); // inf
  } else {
    Map<Integer, Int> num_factors = factor(numerator(x));
    Int val = num_factors.exists(p) ? num_factors[p] : 0;
    Map<Integer, Int> den_factors = factor(denominator(x));
    if (den_factors.exists(p)) val -= den_factors[p];
    return TropicalNumber<Min>(val);
  }
}

// Decomposes n>0 into two integers a,b such that
// - a is square free
// - n = a * b^2
// - a,b > 0
std::pair<Integer, Integer> factor_out_squares(const Integer& n){
   Map<Integer, Int> factorization(factor(n)), root, coeff;
   for(const auto& p : factorization){
      Int ex_curr = p.second;
      if(ex_curr%2 == 1){
         root[p.first] = 1;
         ex_curr--;
      }
      if(ex_curr != 0){
         coeff[p.first] = ex_curr/2;
      }
   }
   return std::make_pair(expand(root), expand(coeff));
}

} // flint
} // pm

namespace polymake {
namespace common {
 
   UserFunction4perl("# @category Utilities"
                     "# Use flint to compute the prime factorization of an Integer"
                     "# @param Integer n"
                     "# @return Map<Integer,Int> pairs of coefficient and exponent",
                     &pm::flint::factor, "factor");
   
   UserFunction4perl("# @category Utilities"
                     "# Use flint to expand the prime factorization of an Integer"
                     "# This is the inverse operation of [[factor]]"
                     "# @param Map<Integer,Int> factorization"
                     "# @return Integer n",
                     &pm::flint::expand, "expand");

   UserFunction4perl("# @category Utilities"
                     "# Use flint's Integer factorization to compute the //p//-adic valuation of a Rational //x//"
		     "# @param Rational x"
		     "# @param Integer p"
                     "# @return TropicalNumber<Min>",
                     &pm::flint::valuation, "valuation");

  
}
}
