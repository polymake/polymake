/* Copyright (c) 1997-2023
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
   constexpr ssize_t small_size = 65;
   char small_buf[small_size];

   if (const char* den = strchr(s, '/')) {
      const ssize_t numerator_digits = den - s;
      if (numerator_digits == 0)
         throw GMP::error("Rational: empty numerator");
      ++den;
      if (!isdigit(*den))
         throw GMP::error("Rational: syntax error in denominator");

      char* num;
      if (numerator_digits < small_size) {
         num = small_buf;
         std::copy(s, s+numerator_digits, num);
         num[numerator_digits] = 0;
      } else {
#ifdef __gnu_linux__
         num = strndup(s, numerator_digits);
         if (!num) throw std::bad_alloc();
#else
         num = new char[numerator_digits+1];
         if (!num) throw std::bad_alloc();
         std::memcpy(num, s, numerator_digits);
         num[numerator_digits] = 0;
#endif
      }
      const bool bad_num = mpz_set_str(mpq_numref(this), num[0] == '+' ? num+1 : num, 0) < 0;
      if (numerator_digits >= small_size) {
#ifdef __gnu_linux__
         free(num);
#else
         delete[] num;
#endif
      }
      if (bad_num)
         throw GMP::error("Rational: syntax error in numerator");
      if (mpz_set_str(mpq_denref(this), den, 0) < 0)
         throw GMP::error("Rational: syntax error in denominator");
      canonicalize();

   } else if (const char* point = strchr(s, '.')) {
      const ssize_t before_pt = point - s;
      ssize_t after_pt = 0;
      ++point;
      ssize_t trailing = 0;
      while (isdigit(point[after_pt])) {
         if (point[after_pt] != '0')
            trailing = after_pt+1;
         ++after_pt;
      }
      char* num;
      if (before_pt+trailing < small_size) {
         num = small_buf;
      } else {
         num = new char[before_pt+trailing+1];
         if (!num) throw std::bad_alloc();
      }
      if (before_pt)
         std::memcpy(num, s, before_pt);
      if (trailing)
         std::memcpy(num+before_pt, point, trailing);
      num[before_pt+trailing] = 0;
      const bool bad_num = mpz_set_str(mpq_numref(this), num[0] == '+' ? num+1 : num, 10) < 0;
      if (before_pt+trailing >= small_size) {
         delete[] num;
      }
      if (bad_num)
         throw GMP::error("Rational: syntax error");
      if (trailing) {
         mpz_ui_pow_ui(mpq_denref(this), 10, trailing);
         canonicalize();
      } else {
         mpz_set_ui(mpq_denref(this), 1);
      }

   } else if (mpz_set_str(mpq_numref(this), s[0] == '+' ? s+1 : s, 0) >= 0) {
      mpz_set_ui(mpq_denref(this), 1);
   } else {
      if (s[0] == '+' ? !strcmp(s+1,"inf") : !strcmp(s,"inf"))
         set_inf(this, 1, initialized::yes);
      else if (s[0] == '-' && !strcmp(s+1, "inf"))
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
   const std::ios::fmtflags flags = os.flags();
   bool show_den = false;
   ssize_t s = numerator(*this).strsize(flags);
   if (!is_integral()) {
      show_den = true;
      s += denominator(*this).strsize(flags);  // '/' occupies the place of the numerator's terminating NULL
   }
   putstr(flags, OutCharBuffer::reserve(os, s), show_den);
}

void Rational::putstr(std::ios::fmtflags flags, char* buf, bool show_den) const
{
   numerator(*this).putstr(flags, buf);
   if (show_den) {
      buf += strlen(buf);
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

mp_limb_t limb0 = 0, limb1 = 1;
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

Rational pow(const Rational& base, long exp)
{
   return Rational::pow(base, exp);
}

}


// If the C++ library that was used to build the GMP C++ bindings is different
// from the one that is used for polymake several bundled libraries might break
// because there are no output/input operators for the GMP C++ types.
// These signatures replicate what is present in gmp.h but when building this
// with a different C++ library this will produce slightly different symbols
// (with / without an extra ::__1)
//
// This is enabled from the configure scripts of bundled/{libnormaliz,sympol}

#ifdef PM_MIXED_OSTREAM

std::ostream& operator<< (std::ostream &s, mpz_srcptr i) {
   pm::Integer pi;
   pi.copy_from(i);
   return s << pi;
}
std::ostream& operator<< (std::ostream &s, mpq_srcptr r) {
   pm::Rational pr;
   pr.copy_from(r);
   return s << pr;
}
std::istream& operator>> (std::istream &s, mpz_ptr i) {
   pm::Integer pi;
   s >> pi;
   mpz_set(i, pi.get_rep());
   return s;
}
std::istream& operator>> (std::istream &s, mpq_ptr r) {
   pm::Rational pr;
   s >> pr;
   mpq_set(r, pr.get_rep());
   return s;
}

#endif

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
