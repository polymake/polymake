/* Copyright (c) 1997-2019
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

#include "polymake/Rational.h"

namespace pm {

Rational::Rational(const char* s)
{
   mpq_init(this);
   try {
      parse(s);
   }
   catch (const GMP::error&) {
      mpq_clear(this);
      throw;
   }
}

void Rational::parse(const char* s)
{
   if (const char* den=strchr(s,'/')) {
      const int numerator_digits=den-s;
      if (!numerator_digits)
         throw GMP::error("Rational: empty numerator");
      ++den;
      if (!isdigit(*den))
         throw GMP::error("Rational: syntax error in denominator");

#ifdef __gnu_linux__
      char *num=strndup(s, numerator_digits);
      if (!num) throw std::bad_alloc();
#else
      char *num=(char*)malloc(numerator_digits+1);
      if (!num) throw std::bad_alloc();
      std::memcpy(num, s, numerator_digits);
      num[numerator_digits]=0;
#endif
      if (mpz_set_str(mpq_numref(this), num, 0) < 0) {
         free(num);
         throw GMP::error("Rational: syntax error in numerator");
      }
      free(num);
      if (mpz_set_str(mpq_denref(this), den, 0) < 0)
         throw GMP::error("Rational: syntax error in denominator");
      canonicalize();

   } else if (const char* point=strchr(s, '.')) {
      const int before_pt=point-s;
      int after_pt=0;
      ++point;
      int trailing=0;
      while (isdigit(point[after_pt])) {
         if (point[after_pt]!='0') trailing=after_pt+1;
         ++after_pt;
      }
      char *num=(char*)malloc(before_pt+trailing+1);
      if (!num) throw std::bad_alloc();
      if (before_pt) std::memcpy(num, s, before_pt);
      if (trailing) std::memcpy(num+before_pt, point, trailing);
      num[before_pt+trailing]=0;
      if (mpz_set_str(mpq_numref(this), num, 10) < 0) {
         free(num);
         throw GMP::error("Rational: syntax error");
      }
      free(num);
      if (trailing) {
         mpz_ui_pow_ui(mpq_denref(this), 10, trailing);
         canonicalize();
      } else {
         mpz_set_ui(mpq_denref(this), 1);
      }

   } else if (mpz_set_str(mpq_numref(this), s, 0) >= 0) {
      mpz_set_ui(mpq_denref(this), 1);
   } else {
      if (s[0]=='+' ? !strcmp(s+1,"inf") : !strcmp(s,"inf"))
         set_inf(this, 1, initialized::yes);
      else if (s[0]=='-' && !strcmp(s+1,"inf"))
         set_inf(this, -1, initialized::yes);
      else
         throw GMP::error("Rational: syntax error");
   }
}

void Rational::read(std::istream& is)
{
   static_cast<Integer*>(mpq_numref(this))->read(is);
   if (!is.eof() && is.peek() == '/') {
      is.ignore();
      static_cast<Integer*>(mpq_denref(this))->read(is, false);
      canonicalize();
   } else {
      // integral number or ±inf
      mpz_set_ui(mpq_denref(this), 1);
   }
}

void Rational::write(std::ostream& os) const
{
   const std::ios::fmtflags flags=os.flags();
   bool show_den=false;
   int s=numerator(*this).strsize(flags);
   if (!is_integral()) {
      show_den=true;
      s+=denominator(*this).strsize(flags);  // '/' occupies the place of the numerator's terminating NULL
   }
   putstr(flags, OutCharBuffer::reserve(os, s), show_den);
}

void Rational::putstr(std::ios::fmtflags flags, char* buf, bool show_den) const
{
   numerator(*this).putstr(flags, buf);
   if (show_den) {
      buf+=strlen(buf);
      *buf++ = '/';
      denominator(*this).putstr(flags & ~std::ios::showpos, buf);
   }
}

void Rational::mult_with_Integer(const Rational& a, const Integer& b)
{
   if (__builtin_expect(!a.is_zero(), 1)) {
      if (__builtin_expect(!b.is_zero(), 1)) {
         mpz_t g;  mpz_init(g);
         mpz_gcd(g, mpq_denref(&a), &b);
         if (!mpz_cmp_ui(g, 1)) {
            mpz_mul(mpq_numref(this), mpq_numref(&a), &b);
            if (this != &a) mpz_set(mpq_denref(this), mpq_denref(&a));
         } else {
            mpz_divexact(mpq_denref(this), mpq_denref(&a), g);
            mpz_divexact(g, &b, g);
            mpz_mul(mpq_numref(this), mpq_numref(&a), g);
         }
         mpz_clear(g);
      } else {
         *this=0;
      }
   }
}

void Rational::div_thru_Integer(const Rational& a, const Integer& b)
{
   if (__builtin_expect(b.is_zero(), 0))
      throw GMP::ZeroDivide();
   if (__builtin_expect(!a.is_zero(), 1)) {
      mpz_t g;  mpz_init(g);
      mpz_gcd(g, mpq_numref(&a), &b);
      if (!mpz_cmp_ui(g, 1)) {
         if (this != &a) mpz_set(mpq_numref(this), mpq_numref(&a));
         mpz_mul(mpq_denref(this), mpq_denref(&a), &b);
      } else {
         mpz_divexact(mpq_numref(this), mpq_numref(&a), g);
         mpz_divexact(g, &b, g);
         mpz_mul(mpq_denref(this), mpq_denref(&a), g);
      }
      canonicalize_sign();
      mpz_clear(g);
   }
}

Rational& Rational::operator*= (long b)
{
   if (__builtin_expect(isfinite(*this), 1)) {
      if (__builtin_expect(!is_zero(), 1)) {
         if (__builtin_expect(b, 1)) {
            unsigned long g=mpz_gcd_ui(nullptr, mpq_denref(this), std::abs(b));
            if (g==1) {
               mpz_mul_si(mpq_numref(this), mpq_numref(this), b);
            } else {
               mpz_divexact_ui(mpq_denref(this), mpq_denref(this), g);
               mpz_mul_si(mpq_numref(this), mpq_numref(this), b/long(g));
            }
         } else {
            *this=0;
         }
      }
   } else {
      inf_inv_sign(this, b);
   }
   return *this;
}

Rational& Rational::operator/= (long b)
{
   if (__builtin_expect(isfinite(*this), 1)) {
      if (__builtin_expect(!b, 0))
         throw GMP::ZeroDivide();
      if (__builtin_expect(!is_zero(), 1)) {
         const unsigned long babs=std::abs(b);
         unsigned long g=mpz_gcd_ui(nullptr, mpq_numref(this), babs);
         if (g==1) {
            mpz_mul_ui(mpq_denref(this), mpq_denref(this), babs);
         } else {
            mpz_divexact_ui(mpq_numref(this), mpq_numref(this), g);
            mpz_mul_ui(mpq_denref(this), mpq_denref(this), babs/g);
         }
         if (b<0) negate();
      }
   } else {
      inf_inv_sign(this, b);
   }
   return *this;
}

namespace {

mp_limb_t limb0=0, limb1=1;
const __mpq_struct mpq_zero_c{ {1, 0, &limb0}, {1, 1, &limb1} };
const __mpq_struct mpq_one_c { {1, 1, &limb1}, {1, 1, &limb1} };

}

const Rational& spec_object_traits<Rational>::zero()
{
   return static_cast<const Rational&>(mpq_zero_c);
}

const Rational& spec_object_traits<Rational>::one()
{
   return static_cast<const Rational&>(mpq_one_c);
}

template <>
Rational
pow(const Rational& base, long exp)
{
   return Rational::pow(base,exp);
}

}

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
