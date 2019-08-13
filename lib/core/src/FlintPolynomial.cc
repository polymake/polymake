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

#ifdef POLYMAKE_WITH_FLINT

#include "polymake/Smith_normal_form.h"
#include "polymake/Polynomial.h"
#include "polymake/FlintPolynomial.h"


namespace pm {


template <>
UniPolynomial<Rational, int>
gcd(const UniPolynomial<Rational, int>& a, const UniPolynomial<Rational, int>& b)
{
   return UniPolynomial<Rational,int>(FlintPolynomial::gcd(*a.impl_ptr,*b.impl_ptr));
}

template <>
ExtGCD< UniPolynomial<Rational, int> >
ext_gcd(const UniPolynomial<Rational, int>& a, const UniPolynomial<Rational, int>& b, bool normalize_gcd)
{
   ExtGCD<UniPolynomial<Rational,int>> res;
   FlintPolynomial::xgcd(*res.g.impl_ptr, *res.p.impl_ptr, *res.q.impl_ptr, *a.impl_ptr, *b.impl_ptr);
   res.k1 = div_exact(a,res.g);
   res.k2 = div_exact(b,res.g);
   return res;
}


}

#endif

