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

#include "polymake/AccurateFloat.h"
#include <iostream>

namespace pm {

Integer::Integer(const AccurateFloat& a)
{
   if (__builtin_expect(mpfr_nan_p(a.rep), 0))
      throw GMP::NaN();
   const int s=isinf(a);
   if (__builtin_expect(s,0)) {
      _init_set_inf(rep, s);
   } else {
      mpz_init(rep);
      mpfr_get_z(rep, a.rep, MPFR_RNDZ);
   }
}

Integer& Integer::operator=(const AccurateFloat& a)
{
   if (__builtin_expect(mpfr_nan_p(a.rep), 0))
      throw GMP::NaN();
   const int s=isinf(a);
   if (__builtin_expect(s,0)) {
      _set_inf(rep, s);
   } else {
      mpfr_get_z(rep, a.rep, MPFR_RNDZ);
   }
   return *this;
}

Rational::Rational(const AccurateFloat& a)
{
   if (__builtin_expect(mpfr_nan_p(a.rep), 0))
      throw GMP::NaN();
   const int s=isinf(a);
   if (__builtin_expect(s, 0)) {
      _init_set_inf(rep, s);
   } else {
      mpq_init(rep);
      if (mpfr_sgn(a.rep)) {
         const mpfr_exp_t e=mpfr_get_z_2exp(mpq_numref(rep), a.rep);
         if (e<0) {
            mpz_mul_2exp(mpq_denref(rep), mpq_denref(rep), -e);
            mpq_canonicalize(rep);
         } else if (e>0) {
            mpz_mul_2exp(mpq_numref(rep), mpq_numref(rep), e);
            mpq_canonicalize(rep);
         }
      }
   }
}

Rational& Rational::operator= (const AccurateFloat& a)
{
   if (__builtin_expect(mpfr_nan_p(a.rep), 0))
      throw GMP::NaN();
   const int s=isinf(a);
   if (__builtin_expect(s, 0)) {
      _set_inf(rep, s);
   } else {
      mpz_set_ui(mpq_denref(rep), 1);
      if (mpfr_sgn(a.rep)) {
         const mpfr_exp_t e=mpfr_get_z_2exp(mpq_numref(rep), a.rep);
         if (e<0) {
            mpz_mul_2exp(mpq_denref(rep), mpq_denref(rep), -e);
            mpq_canonicalize(rep);
         } else if (e>0) {
            mpz_mul_2exp(mpq_numref(rep), mpq_numref(rep), e);
            mpq_canonicalize(rep);
         }
      } else {
         mpz_set_ui(mpq_numref(rep), 0);
      }
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
   if (mpfr_zero_p(rep)) {
      if (mpfr_sgn(rep)<0)
         os.write("-0", 2);
      else if (flags & std::ios::showpos)
         os.write("+0", 2);
      else
         os.put('0');
      return;
   }

   mpfr_exp_t e;
   char* const str0=mpfr_get_str(NULL, &e, 10, 0, rep, MPFR_RNDN);
   char* str=str0;
   const bool neg=mpfr_sgn(rep)<0;
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

}

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
