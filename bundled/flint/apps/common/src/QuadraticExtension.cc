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


#include "polymake/common/factorization.h"
#include "polymake/QuadraticExtension.h"
#include "polymake/Rational.h"
#include "polymake/Rational.h"
#include "polymake/Map.h"

namespace pm {


void reduceQuadratic(Rational& c, Rational& r){
   std::pair <Integer, Integer> num = flint::factor_out_squares(numerator(r));
   std::pair <Integer, Integer> den = flint::factor_out_squares(denominator(r));
   r = Rational(num.first*den.first, 1);
   c *= Rational(num.second, den.second*den.first);
}

template < >
void QuadraticExtension<Rational>::normalize(){
   const Int i1 = isinf(a_);
   const Int i2 = isinf(b_);
   if (__builtin_expect(i1 || i2, 0)) {
      if (i1+i2 == 0) throw GMP::NaN();
      if (!i1) a_ = b_;
      b_ = zero_value<Rational>();
      r_ = zero_value<Rational>();
   } else {
      const Int s = sign(r_);
      if (s < 0)
         throw NonOrderableError();
      if (s == 0)
         b_ = zero_value<Rational>();
      else if (is_zero(b_))
         r_ = zero_value<Rational>();
      else 
         reduceQuadratic(b_, r_);
      if (r_ == 1){
         a_ = a_ + b_;
         b_ = zero_value<Rational>();
         r_ = zero_value<Rational>();
      }
   }
}
   
} //End namespace pm


