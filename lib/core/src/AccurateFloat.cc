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

#include "polymake/AccurateFloat.h"
#include <iostream>

namespace pm {

Integer& Integer::operator= (const AccurateFloat& b)
{
   if (__builtin_expect(mpfr_nan_p(&b), 0))
      throw GMP::NaN();

   const int s=isinf(b);
   if (__builtin_expect(!s, 1))
      mpfr_get_z(this, &b, MPFR_RNDZ);
   else
      set_inf(this, s);
   return *this;
}

Rational& Rational::operator= (const AccurateFloat& b)
{
   if (__builtin_expect(mpfr_nan_p(&b), 0))
      throw GMP::NaN();

   if (__builtin_expect(isfinite(b), 1)) {
      if (b.is_zero()) {
         *this=0;
      } else {
         const mpfr_exp_t e=mpfr_get_z_2exp(mpq_numref(this), &b);
         mpz_set_ui(mpq_denref(this), 1);
         if (e<0) {
            mpz_mul_2exp(mpq_denref(this), mpq_denref(this), -e);
            mpq_canonicalize(this);
         } else if (e>0) {
            mpz_mul_2exp(mpq_numref(this), mpq_numref(this), e);
            mpq_canonicalize(this);
         }
      }
   } else {
      set_inf(this, isinf(b));
   }
   return *this;
}

void AccurateFloat::putstr(std::ostream& os, std::ios::fmtflags flags) const
{
   const int i=isinf(*this);
   if (__builtin_expect(i,0)) {
      if (i<0)
         os.write("-inf", 4);
      else if (flags & std::ios::showpos)
         os.write("+inf", 4);
      else
         os.write("inf", 3);
      return;
   }
   if (is_zero()) {
      if (mpfr_sgn(this)<0)
         os.write("-0", 2);
      else if (flags & std::ios::showpos)
         os.write("+0", 2);
      else
         os.put('0');
      return;
   }

   mpfr_exp_t e;
   char* const str0=mpfr_get_str(nullptr, &e, 10, 0, this, MPFR_RNDN);
   char* str=str0;
   const bool neg=mpfr_sgn(this)<0;
   const int l=strlen(str);
   const int digits=l-neg;

   if (neg)
      os.put(*str++);   // '-' sign
   else if (flags & std::ios::showpos)
      os.put('+');

   if (e<-3) {
      os << *str++ << '.';
      os.write(str, digits-1) << 'e' << e-1;
   } else if (e<=0) {
      os << '0' << '.';
      for (; e<0; ++e) os << '0';
      os.write(str, digits);
   } else if (e<digits) {
      os.write(str, e) << '.';
      os.write(str+e, digits-e);
   } else if (e==digits) {
      os.write(str, digits);
   } else {
      os << *str++ << '.';
      os.write(str, digits-1) << 'e' << e-1;
   }
   mpfr_free_str(str0);
}

void AccurateFloat::read(std::istream& is)
{
   std::string rep;
   is >> rep;
   if (mpfr_set_str(this, rep.c_str(), 10, MPFR_RNDN))
      throw std::runtime_error("AccurateFloat: Could not parse '" + rep + "'");
}

const AccurateFloat& spec_object_traits<AccurateFloat>::zero()
{
   const static AccurateFloat z(0);
   return z;
}

const AccurateFloat& spec_object_traits<AccurateFloat>::one()
{
   const static AccurateFloat e(1);
   return e;
}

template <>
AccurateFloat
pow(const AccurateFloat& base, long exp)
{
   return AccurateFloat::pow(base,exp);
}


}

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
