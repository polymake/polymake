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

#ifdef POLYMAKE_WITH_FLINT

#include "polymake/Smith_normal_form.h"
#include "polymake/Polynomial.h"
#include "polymake/FlintPolynomial.h"

namespace pm {

struct flint_cleaner {
   flint_cleaner() {}
   ~flint_cleaner() {
#pragma omp parallel
      flint_cleanup();
   }
};

flint_cleaner flint_cleanup_helper;

template <>
UniPolynomial<Rational, Int>
gcd(const UniPolynomial<Rational, Int>& a, const UniPolynomial<Rational, Int>& b)
{
   return UniPolynomial<Rational, Int>(FlintPolynomial::gcd(*a.impl_ptr, *b.impl_ptr));
}

template <>
ExtGCD< UniPolynomial<Rational, Int> >
ext_gcd(const UniPolynomial<Rational, Int>& a, const UniPolynomial<Rational, Int>& b, bool normalize_gcd)
{
   ExtGCD<UniPolynomial<Rational, Int>> res;
   FlintPolynomial::xgcd(*res.g.impl_ptr, *res.p.impl_ptr, *res.q.impl_ptr, *a.impl_ptr, *b.impl_ptr);
   res.k1 = div_exact(a,res.g);
   res.k2 = div_exact(b,res.g);
   return res;
}


}

#endif

