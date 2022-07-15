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

#pragma once

#include "polymake/Rational.h"
#include <mpfr.h>

namespace pm {

bool isfinite(const AccurateFloat& a) noexcept;
Int isinf(const AccurateFloat& a) noexcept;
Int sign(const AccurateFloat& a) noexcept;

/// minimalistic wrapper for MPFR numbers
class AccurateFloat
   : protected __mpfr_struct {
public:
   ~AccurateFloat() noexcept
   {
      if (_mpfr_d) mpfr_clear(this);
   }

   /// Constructors

   AccurateFloat(const AccurateFloat& b)
   {
      mpfr_init_set(this, &b, MPFR_RNDN);
   }

   AccurateFloat(AccurateFloat&& b) noexcept
   {
      static_cast<__mpfr_struct&>(*this) = b;
      b._mpfr_d = nullptr;
   }

   explicit AccurateFloat(const Integer& b)
   {
      if (__builtin_expect(isfinite(b), 1)) {
         mpfr_init_set_z(this, &b, MPFR_RNDZ);
      } else {
         mpfr_init(this);
         mpfr_set_inf(this, int(sign(b)));
      }
   }

   explicit AccurateFloat(const Rational& b)
   {
      if (__builtin_expect(isfinite(b), 1)) {
         mpfr_init_set_q(this, &b, MPFR_RNDN);
      } else {
         mpfr_init(this);
         mpfr_set_inf(this, int(sign(b)));
      }
   }

   explicit AccurateFloat(long b = 0)
   {
      mpfr_init_set_si(this, b, MPFR_RNDZ);
   }

   explicit AccurateFloat(int b)
      : AccurateFloat(long(b)) {}

   explicit AccurateFloat(double b)
   {
      mpfr_init_set_d(this, b, MPFR_RNDN);
   }

   explicit AccurateFloat(gmp_randstate_t rnd)
   {
      mpfr_init(this);
      mpfr_urandom(this, rnd, MPFR_RNDZ);
   }

   /// Copy the value from a third party
   explicit AccurateFloat(const __mpfr_struct& b)
   {
      mpfr_set(this, &b, MPFR_RNDN);
   }

   /// Construct an infinite value with the given sign
   static
   AccurateFloat infinity(int s)
   {
      assert(s == 1 || s == -1);
      AccurateFloat result;
      mpfr_set_inf(&result, s);
      return result;
   }

   AccurateFloat& operator= (const AccurateFloat& b)
   {
      mpfr_set(this, &b, MPFR_RNDN);
      return *this;
   }

   AccurateFloat& operator= (AccurateFloat&& b)
   {
      mpfr_swap(this, &b);
      return *this;
   }

   AccurateFloat& operator= (const Rational& b)
   {
      const Int s2 = isinf(b);
      if (__builtin_expect(s2, 0))
         mpfr_set_inf(this, int(s2));
      else
         mpfr_set_q(this, &b, MPFR_RNDN);
      return *this;
   }

   AccurateFloat& operator= (const Integer& b)
   {
      const Int s2 = isinf(b);
      if (__builtin_expect(s2, 0))
         mpfr_set_inf(this, int(s2));
      else
         mpfr_set_z(this, &b, MPFR_RNDZ);
      return *this;
   }

   AccurateFloat& operator= (long b)
   {
      mpfr_set_si(this, b, MPFR_RNDZ);
      return *this;
   }

   AccurateFloat& operator= (int b)
   {
      return *this = long(b);
   }

   AccurateFloat& operator= (double b)
   {
      mpfr_set_d(this, b, MPFR_RNDN);
      return *this;
   }

   AccurateFloat& copy_from(mpfr_srcptr src)
   {
      mpfr_set(this, src, MPFR_RNDN);
      return *this;
   }

   AccurateFloat& swap_with(mpfr_ptr src)
   {
      mpfr_swap(this, src);
      return *this;
   }

   void swap(AccurateFloat& b)
   {
      mpfr_swap(this, &b);
   }

   // TODO: kill this
   friend void relocate(AccurateFloat* from, AccurateFloat* to)
   {
      static_cast<__mpfr_struct&>(*to)=*from;
   }

   explicit operator double() const
   {
      return mpfr_get_d(this, MPFR_RNDN);
   }

   explicit operator long() const
   {
      return mpfr_get_si(this, MPFR_RNDN);
   }

   AccurateFloat& set_random(gmp_randstate_t rnd)
   {
      mpfr_urandom(this, rnd, MPFR_RNDZ);
      return *this;
   }

   void set_precision(mpfr_prec_t precision)
   {
      assert(precision >= 2);
      mpfr_prec_round(this, precision, MPFR_RNDN);
   }

   mpfr_prec_t get_precision() const
   {
      return mpfr_get_prec(this);
   }

   friend
   AccurateFloat operator+ (const AccurateFloat& a)
   {
      return a;
   }

   friend
   AccurateFloat&& operator+ (AccurateFloat&& a)
   {
      return std::move(a);
   }

   AccurateFloat& operator++ ()
   {
      mpfr_add_si(this, this, 1, MPFR_RNDZ);
      return *this;
   }

   AccurateFloat& operator-- ()
   {
      mpfr_sub_si(this, this, 1, MPFR_RNDZ);
      return *this;
   }

   AccurateFloat& negate()
   {
      mpfr_neg(this, this, MPFR_RNDN);
      return *this;
   }

   friend
   AccurateFloat operator- (const AccurateFloat& a)
   {
      AccurateFloat result(a);
      result.negate();
      return result;
   }

   friend
   AccurateFloat&& operator- (AccurateFloat&& a)
   {
      return std::move(a.negate());
   }

   AccurateFloat& operator+= (const AccurateFloat& b)
   {
      mpfr_add(this, this, &b, MPFR_RNDN);
      return *this;
   }

   friend
   AccurateFloat operator+ (const AccurateFloat& a, const AccurateFloat& b)
   {
      AccurateFloat result;
      mpfr_add(&result, &a, &b, MPFR_RNDN);
      return result;
   }

   friend
   AccurateFloat&& operator+ (AccurateFloat&& a, const AccurateFloat& b)
   {
      return std::move(a+=b);
   }
   friend
   AccurateFloat&& operator+ (const AccurateFloat& a, AccurateFloat&& b)
   {
      return std::move(b+=a);
   }
   friend
   AccurateFloat&& operator+ (AccurateFloat&& a, AccurateFloat&& b)
   {
      return a._mpfr_prec >= b._mpfr_prec ? std::move(a+=b) : std::move(b+=a);
   }

   AccurateFloat& operator+= (const Integer& b)
   {
      if (!add_inf(isinf(b)))
         mpfr_add_z(this, this, b.get_rep(), MPFR_RNDN);
      return *this;
   }

   friend
   AccurateFloat operator+ (const AccurateFloat& a, const Integer& b)
   {
      AccurateFloat result;
      if (!result.init_add_inf(a, isinf(b)))
         mpfr_add_z(&result, &a, b.get_rep(), MPFR_RNDN);
      return result;
   }

   friend
   AccurateFloat&& operator+ (AccurateFloat&& a, const Integer& b)
   {
      return std::move(a+=b);
   }
   friend
   AccurateFloat operator+ (const Integer& a, const AccurateFloat& b)
   {
      return b+a;
   }
   friend
   AccurateFloat&& operator+ (const Integer& a, AccurateFloat&& b)
   {
      return std::move(b+=a);
   }

   AccurateFloat& operator+= (const Rational& b)
   {
      if (!add_inf(isinf(b)))
         mpfr_add_q(this, this, b.get_rep(), MPFR_RNDN);
      return *this;
   }

   friend
   AccurateFloat operator+ (const AccurateFloat& a, const Rational& b)
   {
      AccurateFloat result;
      if (!result.init_add_inf(a, isinf(b)))
         mpfr_add_q(&result, &a, b.get_rep(), MPFR_RNDN);
      return result;
   }

   friend
   AccurateFloat&& operator+ (AccurateFloat&& a, const Rational& b)
   {
      return std::move(a+=b);
   }
   friend
   AccurateFloat operator+ (const Rational& a, const AccurateFloat& b)
   {
      return b+a;
   }
   friend
   AccurateFloat&& operator+ (const Rational& a, AccurateFloat&& b)
   {
      return std::move(b+=a);
   }

   AccurateFloat& operator+= (long b)
   {
      mpfr_add_si(this, this, b, MPFR_RNDN);
      return *this;
   }
   AccurateFloat& operator+= (int b)
   {
      return *this += long(b);
   }

   friend
   AccurateFloat operator+ (const AccurateFloat& a, long b)
   {
      AccurateFloat result;
      mpfr_add_si(&result, &a, b, MPFR_RNDN);
      return result;
   }

   friend
   AccurateFloat&& operator+ (AccurateFloat&& a, long b)
   {
      return std::move(a += b);
   }
   friend
   AccurateFloat operator+ (long a, const AccurateFloat& b)
   {
      return b+a;
   }
   friend
   AccurateFloat operator+ (long a, AccurateFloat&& b)
   {
      return std::move(b += a);
   }
   friend
   AccurateFloat operator+ (const AccurateFloat& a, int b)
   {
      return a+long(b);
   }
   friend
   AccurateFloat&& operator+ (AccurateFloat&& a, int b)
   {
      return std::move(a += long(b));
   }
   friend AccurateFloat operator+ (int a, const AccurateFloat& b)
   {
      return b+long(a);
   }
   friend
   AccurateFloat operator+ (int a, AccurateFloat&& b)
   {
      return std::move(b += long(a));
   }

   AccurateFloat& operator+= (double b)
   {
      mpfr_add_d(this, this, b, MPFR_RNDN);
      return *this;
   }

   friend
   AccurateFloat operator+ (const AccurateFloat& a, double b)
   {
      AccurateFloat result;
      mpfr_add_d(&result, &a, b, MPFR_RNDN);
      return result;
   }

   friend
   AccurateFloat&& operator+ (AccurateFloat&& a, double b)
   {
      return std::move(a+=b);
   }
   friend
   AccurateFloat operator+ (double a, const AccurateFloat& b)
   {
      return b+a;
   }
   friend
   AccurateFloat&& operator+ (double a, AccurateFloat&& b)
   {
      return std::move(b+=a);
   }

   AccurateFloat& operator-= (const AccurateFloat& b)
   {
      mpfr_sub(this, this, &b, MPFR_RNDN);
      return *this;
   }

   friend
   AccurateFloat operator- (const AccurateFloat& a, const AccurateFloat& b)
   {
      AccurateFloat result;
      mpfr_sub(&result, &a, &b, MPFR_RNDN);
      return result;
   }

   friend
   AccurateFloat&& operator- (AccurateFloat&& a, const AccurateFloat& b)
   {
      return std::move(a-=b);
   }
   friend
   AccurateFloat&& operator- (const AccurateFloat& a, AccurateFloat&& b)
   {
      return std::move((b-=a).negate());
   }
   friend
   AccurateFloat&& operator- (AccurateFloat&& a, AccurateFloat&& b)
   {
      return a._mpfr_prec >= b._mpfr_prec ? std::move(a -= b) : std::move((b -= a).negate());
   }

   AccurateFloat& operator-= (const Integer& b)
   {
      if (!add_inf(-isinf(b)))
         mpfr_sub_z(this, this, b.get_rep(), MPFR_RNDN);
      return *this;
   }

   friend
   AccurateFloat operator- (const AccurateFloat& a, const Integer& b)
   {
      AccurateFloat result;
      if (!result.init_add_inf(a, -isinf(b)))
         mpfr_sub_z(&result, &a, b.get_rep(), MPFR_RNDN);
      return result;
   }

   friend
   AccurateFloat&& operator- (AccurateFloat&& a, const Integer& b)
   {
      return std::move(a -= b);
   }
   friend
   AccurateFloat operator- (const Integer& a, const AccurateFloat& b)
   {
      return (b-a).negate();
   }
   friend
   AccurateFloat&& operator- (const Integer& a, AccurateFloat&& b)
   {
      return std::move((b -= a).negate());
   }

   AccurateFloat& operator-= (const Rational& b)
   {
      if (!add_inf(-isinf(b)))
         mpfr_sub_q(this, this, b.get_rep(), MPFR_RNDN);
      return *this;
   }

   friend AccurateFloat operator- (const AccurateFloat& a, const Rational& b)
   {
      AccurateFloat result;
      if (!result.init_add_inf(a, -isinf(b)))
         mpfr_sub_q(&result, &a, b.get_rep(), MPFR_RNDN);
      return result;
   }

   friend
   AccurateFloat&& operator- (AccurateFloat&& a, const Rational& b)
   {
      return std::move(a -= b);
   }
   friend
   AccurateFloat operator- (const Rational& a, const AccurateFloat& b)
   {
      return (b-a).negate();
   }
   friend
   AccurateFloat&& operator- (const Rational& a, AccurateFloat&& b)
   {
      return std::move((b -= a).negate());
   }

   AccurateFloat& operator-= (long b)
   {
      mpfr_sub_si(this, this, b, MPFR_RNDN);
      return *this;
   }
   AccurateFloat& operator-= (int b)
   {
      return *this -= long(b);
   }

   friend
   AccurateFloat operator- (const AccurateFloat& a, long b)
   {
      AccurateFloat result;
      mpfr_sub_si(&result, &a, b, MPFR_RNDN);
      return result;
   }

   friend
   AccurateFloat&& operator- (AccurateFloat&& a, long b)
   {
      return std::move(a -= b);
   }

   friend
   AccurateFloat operator- (long a, const AccurateFloat& b)
   {
      AccurateFloat result;
      mpfr_si_sub(&result, a, &b, MPFR_RNDN);
      return result;
   }

   friend
   AccurateFloat&& operator- (long a, AccurateFloat&& b)
   {
      mpfr_si_sub(&b, a, &b, MPFR_RNDN);
      return std::move(b);
   }

   friend
   AccurateFloat operator- (const AccurateFloat& a, int b)
   {
      return a-long(b);
   }
   friend
   AccurateFloat&& operator- (AccurateFloat&& a, int b)
   {
      return std::move(a -= long(b));
   }
   friend
   AccurateFloat operator- (int a, const AccurateFloat& b)
   {
      return long(a) - b;
   }
   friend
   AccurateFloat&& operator- (int a, AccurateFloat&& b)
   {
      return std::move(long(a) - std::move(b));
   }

   AccurateFloat& operator-= (double b)
   {
      mpfr_sub_d(this, this, b, MPFR_RNDN);
      return *this;
   }

   friend
   AccurateFloat operator- (const AccurateFloat& a, double b)
   {
      AccurateFloat result;
      mpfr_sub_d(&result, &a, b, MPFR_RNDN);
      return result;
   }

   friend
   AccurateFloat&& operator- (AccurateFloat&& a, double b)
   {
      return std::move(a-=b);
   }

   friend
   AccurateFloat operator- (double a, const AccurateFloat& b)
   {
      AccurateFloat result;
      mpfr_d_sub(&result, a, &b, MPFR_RNDN);
      return result;
   }

   friend
   AccurateFloat&& operator- (double a, AccurateFloat&& b)
   {
      mpfr_d_sub(&b, a, &b, MPFR_RNDN);
      return std::move(b);
   }

   AccurateFloat& operator*= (const AccurateFloat& b)
   {
      mpfr_mul(this, this, &b, MPFR_RNDN);
      return *this;
   }

   friend
   AccurateFloat operator* (const AccurateFloat& a, const AccurateFloat& b)
   {
      AccurateFloat result;
      mpfr_mul(&result, &a, &b, MPFR_RNDN);
      return result;
   }

   friend
   AccurateFloat&& operator* (AccurateFloat&& a, const AccurateFloat& b)
   {
      return std::move(a *= b);
   }
   friend
   AccurateFloat&& operator* (const AccurateFloat& a, AccurateFloat&& b)
   {
      return std::move(b *= a);
   }
   friend
   AccurateFloat&& operator* (AccurateFloat&& a, AccurateFloat&& b)
   {
      return std::move(a *= b);
   }

   AccurateFloat& operator*= (const Integer& b)
   {
      if (!mul_inf(isinf(b)))
         mpfr_mul_z(this, this, b.get_rep(), MPFR_RNDN);
      return *this;
   }

   friend
   AccurateFloat operator* (const AccurateFloat& a, const Integer& b)
   {
      AccurateFloat result;
      if (!result.init_mul_inf(a, isinf(b)))
         mpfr_mul_z(&result, &a, b.get_rep(), MPFR_RNDN);
      return result;
   }

   friend
   AccurateFloat&& operator* (AccurateFloat&& a, const Integer& b)
   {
      return std::move(a *= b);
   }
   friend
   AccurateFloat operator* (const Integer& a, const AccurateFloat& b)
   {
      return b*a;
   }
   friend
   AccurateFloat&& operator* (const Integer& a, AccurateFloat&& b)
   {
      return std::move(b *= a);
   }

   AccurateFloat& operator*= (const Rational& b)
   {
      if (!mul_inf(isinf(b)))
         mpfr_mul_q(this, this, b.get_rep(), MPFR_RNDN);
      return *this;
   }

   friend
   AccurateFloat operator* (const AccurateFloat& a, const Rational& b)
   {
      AccurateFloat result;
      if (!result.init_mul_inf(a, isinf(b)))
         mpfr_mul_q(&result, &a, b.get_rep(), MPFR_RNDN);
      return result;
   }

   friend
   AccurateFloat&& operator* (AccurateFloat&& a, const Rational& b)
   {
      return std::move(a *= b);
   }
   friend
   AccurateFloat operator* (const Rational& a, const AccurateFloat& b)
   {
      return b*a;
   }
   friend
   AccurateFloat&& operator* (const Rational& a, AccurateFloat&& b)
   {
      return std::move(b *= a);
   }

   AccurateFloat& operator*= (long b)
   {
      mpfr_mul_si(this, this, b, MPFR_RNDN);
      return *this;
   }
   AccurateFloat& operator*= (int b)
   {
      return *this *= long(b);
   }

   friend
   AccurateFloat operator* (const AccurateFloat& a, long b)
   {
      AccurateFloat result;
      mpfr_mul_si(&result, &a, b, MPFR_RNDN);
      return result;
   }

   friend
   AccurateFloat&& operator* (AccurateFloat&& a, long b)
   {
      return std::move(a *= b);
   }
   friend
   AccurateFloat operator* (long a, const AccurateFloat& b)
   {
      return b*a;
   }
   friend
   AccurateFloat&& operator* (long a, AccurateFloat&& b)
   {
      return std::move(b *= a);
   }
   friend
   AccurateFloat operator* (const AccurateFloat& a, int b)
   {
      return a*long(b);
   }
   friend
   AccurateFloat&& operator* (AccurateFloat&& a, int b)
   {
      return std::move(a *= long(b));
   }
   friend
   AccurateFloat operator* (int a, const AccurateFloat& b)
   {
      return b*long(a);
   }
   friend
   AccurateFloat&& operator* (int a, AccurateFloat&& b)
   {
      return std::move(b *= long(a));
   }

   AccurateFloat& operator*= (double b)
   {
      mpfr_mul_d(this, this, b, MPFR_RNDN);
      return *this;
   }

   friend AccurateFloat operator* (const AccurateFloat& a, double b)
   {
      AccurateFloat result;
      mpfr_mul_d(&result, &a, b, MPFR_RNDN);
      return result;
   }

   friend
   AccurateFloat&& operator* (AccurateFloat&& a, double b)
   {
      return std::move(a *= b);
   }
   friend AccurateFloat operator* (double a, const AccurateFloat& b)
   {
      return b*a;
   }
   friend
   AccurateFloat&& operator* (double a, AccurateFloat&& b)
   {
      return std::move(b *= a);
   }

   AccurateFloat& operator/= (const AccurateFloat& b)
   {
      mpfr_div(this, this, &b, MPFR_RNDN);
      return *this;
   }

   friend
   AccurateFloat operator/ (const AccurateFloat& a, const AccurateFloat& b)
   {
      AccurateFloat result;
      mpfr_div(&result, &a, &b, MPFR_RNDN);
      return result;
   }

   friend
   AccurateFloat&& operator/ (AccurateFloat&& a, const AccurateFloat& b)
   {
      return std::move(a /= b);
   }

   AccurateFloat& operator/= (const Integer& b)
   {
      if (!div_inf(isinf(b)))
         mpfr_div_z(this, this, b.get_rep(), MPFR_RNDN);
      return *this;
   }

   friend
   AccurateFloat operator/ (const AccurateFloat& a, const Integer& b)
   {
      AccurateFloat result;
      if (!result.init_div_inf(a, isinf(b)))
         mpfr_div_z(&result, &a, b.get_rep(), MPFR_RNDN);
      return result;
   }

   friend
   AccurateFloat&& operator/ (AccurateFloat&& a, const Integer& b)
   {
      return std::move(a /= b);
   }
   friend
   AccurateFloat operator/ (const Integer& a, const AccurateFloat& b)
   {
      return AccurateFloat(a) / b;
   }

   AccurateFloat& operator/= (const Rational& b)
   {
      if (!div_inf(isinf(b)))
         mpfr_div_q(this, this, b.get_rep(), MPFR_RNDN);
      return *this;
   }

   friend
   AccurateFloat operator/ (const AccurateFloat& a, const Rational& b)
   {
      AccurateFloat result;
      if (!result.init_div_inf(a, isinf(b)))
         mpfr_div_q(&result, &a, b.get_rep(), MPFR_RNDN);
      return result;
   }

   friend
   AccurateFloat&& operator/ (AccurateFloat&& a, const Rational& b)
   {
      return std::move(a /= b);
   }
   friend AccurateFloat operator/ (const Rational& a, const AccurateFloat& b)
   {
      return AccurateFloat(a) / b;
   }

   AccurateFloat& operator/= (long b)
   {
      mpfr_div_si(this, this, b, MPFR_RNDN);
      return *this;
   }
   AccurateFloat& operator/= (int b)
   {
      return *this /= long(b);
   }

   friend
   AccurateFloat operator/ (const AccurateFloat& a, long b)
   {
      AccurateFloat result;
      mpfr_div_si(&result, &a, b, MPFR_RNDN);
      return result;
   }

   friend
   AccurateFloat&& operator/ (AccurateFloat&& a, long b)
   {
      return std::move(a /= b);
   }

   friend
   AccurateFloat operator/ (long a, const AccurateFloat& b)
   {
      AccurateFloat result;
      mpfr_si_div(&result, a, &b, MPFR_RNDN);
      return result;
   }

   friend
   AccurateFloat&& operator/ (long a, AccurateFloat&& b)
   {
      mpfr_si_div(&b, a, &b, MPFR_RNDN);
      return std::move(b);
   }

   friend
   AccurateFloat operator/ (const AccurateFloat& a, int b)
   {
      return a/long(b);
   }
   friend
   AccurateFloat&& operator/ (AccurateFloat&& a, int b)
   {
      return std::move(a /= long(b));
   }
   friend
   AccurateFloat operator/ (int a, const AccurateFloat& b)
   {
      return long(a) / b;
   }
   friend
   AccurateFloat&& operator/ (int a, AccurateFloat&& b)
   {
      return std::move(long(a) / std::move(b));
   }

   AccurateFloat& operator/= (double b)
   {
      mpfr_div_d(this, this, b, MPFR_RNDN);
      return *this;
   }

   friend
   AccurateFloat operator/ (const AccurateFloat& a, double b)
   {
      AccurateFloat result;
      mpfr_div_d(&result, &a, b, MPFR_RNDN);
      return result;
   }

   friend
   AccurateFloat&& operator/ (AccurateFloat&& a, double b)
   {
      return std::move(a /= b);
   }

   friend
   AccurateFloat operator/ (double a, const AccurateFloat& b)
   {
      AccurateFloat result;
      mpfr_d_div(&result, a, &b, MPFR_RNDN);
      return result;
   }

   friend
   AccurateFloat&& operator/ (double a, AccurateFloat&& b)
   {
      mpfr_d_div(&b, a, &b, MPFR_RNDN);
      return std::move(b);
   }

   AccurateFloat& operator<<= (long k)
   {
      if (k >= 0)
         mpfr_mul_2ui(this, this, k, MPFR_RNDZ);
      else
         mpfr_div_2ui(this, this, -k, MPFR_RNDZ);
      return *this;
   }
   AccurateFloat& operator<<= (int k)
   {
      return *this <<= long(k);
   }
   AccurateFloat& operator>>= (long k)
   {
      return *this <<= -k;
   }
   AccurateFloat& operator>>= (int k)
   {
      return *this <<= long(-k);
   }

   friend
   AccurateFloat operator<< (const AccurateFloat& a, long k)
   {
      AccurateFloat result;
      if (k >= 0)
         mpfr_mul_2ui(&result, &a, k, MPFR_RNDZ);
      else
         mpfr_div_2ui(&result, &a, k, MPFR_RNDZ);
      return result;
   }

   friend
   AccurateFloat&& operator<< (AccurateFloat&& a, long k)
   {
      return std::move(a <<= k);
   }
   friend
   AccurateFloat operator<< (const AccurateFloat& a, int k)
   {
      return a << long(k);
   }
   friend
   AccurateFloat&& operator<< (AccurateFloat&& a, int k)
   {
      return std::move(a <<= long(k));
   }

   friend
   AccurateFloat operator>> (const AccurateFloat& a, long k)
   {
      return a << -k;
   }
   friend
   AccurateFloat&& operator>> (AccurateFloat&& a, long k)
   {
      return std::move(a >>= k);
   }
   friend
   AccurateFloat operator>> (const AccurateFloat& a, int k)
   {
      return a << long(-k);
   }
   friend
   AccurateFloat&& operator>> (AccurateFloat&& a, int k)
   {
      return std::move(a >>= long(k));
   }

   friend
   AccurateFloat floor(const AccurateFloat& a)
   {
      AccurateFloat result;
      mpfr_floor(&result, &a);
      return result;
   }

   friend
   AccurateFloat&& floor(AccurateFloat&& a)
   {
      mpfr_floor(&a, &a);
      return std::move(a);
   }

   friend
   AccurateFloat ceil(const AccurateFloat& a)
   {
      AccurateFloat result;
      mpfr_ceil(&result, &a);
      return result;
   }

   friend
   AccurateFloat&& ceil(AccurateFloat&& a)
   {
      mpfr_ceil(&a, &a);
      return std::move(a);
   }

private:
   long round_impl(mpfr_rnd_t rnd) const;
   AccurateFloat round_if_integer_impl(bool& is_rounded, double prec, mpfr_rnd_t rnd) const;

public:
   /// return an AccurateFloat with integral value closest to the given one
   /// @param rnd rounding direction
   friend
   long round(const AccurateFloat& a, mpfr_rnd_t rnd = MPFR_RNDN)
   {
      return a.round_impl(rnd);
   }

   /// return an AccurateFloat with integral value closest to the given one, if the difference lies within given threshold
   /// @param[out] is_rounded set to true if the result is rounded to an integral
   /// @param rnd rounding direction
   friend
   AccurateFloat round_if_integer(const AccurateFloat& a, bool& is_rounded, double prec = 1e-10, mpfr_rnd_t rnd = MPFR_RNDN)
   {
      return a.round_if_integer_impl(is_rounded, prec, rnd);
   }

   friend
   AccurateFloat abs(const AccurateFloat& a)
   {
      AccurateFloat result;
      mpfr_abs(&result, &a, MPFR_RNDN);
      return result;
   }

   friend
   AccurateFloat&& abs(AccurateFloat&& a)
   {
      mpfr_abs(&a, &a, MPFR_RNDN);
      return std::move(a);
   }

   static
   AccurateFloat pow(const AccurateFloat& a, const AccurateFloat& b)
   {
      AccurateFloat result;
      mpfr_pow(&result, &a, &b, MPFR_RNDN);
      return result;
   }

   static
   AccurateFloat&& pow(AccurateFloat&& a, const AccurateFloat& b)
   {
      mpfr_pow(&a, &a, &b, MPFR_RNDN);
      return std::move(a);
   }

   static
   AccurateFloat pow(const AccurateFloat& a, long k)
   {
      AccurateFloat result;
      mpfr_pow_si(&result, &a, k, MPFR_RNDN);
      return result;
   }

   static
   AccurateFloat pow(const AccurateFloat& a, int k)
   {
      return pow(a, long(k));
   }

   static
   AccurateFloat&& pow(AccurateFloat&& a, long k)
   {
      mpfr_pow_si(&a, &a, k, MPFR_RNDN);
      return std::move(a);
   }

   static
   AccurateFloat&& pow(AccurateFloat&& a, int k)
   {
      return std::move(pow(std::move(a), long(k)));
   }

   static
   AccurateFloat pow(long a, long k)
   {
      const bool neg = a < 0 ? (a = -a, true) : false;
      const bool inv = k < 0 ? (k = -k, true) : false;
      AccurateFloat result;
      mpfr_ui_pow_ui(&result, a, k, MPFR_RNDZ);
      if (inv) mpfr_si_div(&result, 1L, &result, MPFR_RNDN);
      if (neg && k%2) result.negate();
      return result;
   }

   static
   AccurateFloat pow(long a, int k)
   {
      return pow(a, long(k));
   }
   static
   AccurateFloat pow(int a, long k)
   {
      return pow(long(a), k);
   }
   static
   AccurateFloat pow(int a, int k)
   {
      return pow(long(a), long(k));
   }

   static
   AccurateFloat pow(long a, const AccurateFloat& b)
   {
      AccurateFloat result;
      if (a >= 0)
         mpfr_ui_pow(&result, a, &b, MPFR_RNDN);
      return result;
   }

   static
   AccurateFloat pow(int a, const AccurateFloat& b)
   {
      return pow(long(a), b);
   }

   friend
   AccurateFloat sqrt(const AccurateFloat& a)
   {
      AccurateFloat result;
      mpfr_sqrt(&result, &a, MPFR_RNDN);
      return result;
   }

   friend
   AccurateFloat&& sqrt(AccurateFloat&& a)
   {
      mpfr_sqrt(&a, &a, MPFR_RNDN);
      return std::move(a);
   }

   friend
   AccurateFloat exp(const AccurateFloat& a)
   {
      AccurateFloat result;
      mpfr_exp(&result, &a, MPFR_RNDN);
      return result;
   }

   friend
   AccurateFloat&& exp(AccurateFloat&& a)
   {
      mpfr_exp(&a, &a, MPFR_RNDN);
      return std::move(a);
   }

   friend
   AccurateFloat log(const AccurateFloat& a)
   {
      AccurateFloat result;
      mpfr_log(&result, &a, MPFR_RNDN);
      return result;
   }

   friend
   AccurateFloat&& log(AccurateFloat&& a)
   {
      mpfr_log(&a, &a, MPFR_RNDN);
      return std::move(a);
   }

   // basic trigonometric functions

   static
   AccurateFloat pi()
   {
      AccurateFloat result;
      mpfr_const_pi(&result, MPFR_RNDN);
      return result;
   }

   friend
   AccurateFloat sin(const AccurateFloat& a)
   {
      AccurateFloat result;
      mpfr_sin(&result, &a, MPFR_RNDN);
      return result;
   }

   friend
   AccurateFloat&& sin(AccurateFloat&& a)
   {
      mpfr_sin(&a, &a, MPFR_RNDN);
      return std::move(a);
   }

   friend
   AccurateFloat cos(const AccurateFloat& a)
   {
      AccurateFloat result;
      mpfr_cos(&result, &a, MPFR_RNDN);
      return result;
   }

   friend
   AccurateFloat&& cos(AccurateFloat&& a)
   {
      mpfr_cos(&a, &a, MPFR_RNDN);
      return std::move(a);
   }

   friend
   AccurateFloat tan(const AccurateFloat& a)
   {
      AccurateFloat result;
      mpfr_tan(&result, &a, MPFR_RNDN);
      return result;
   }

   friend
   AccurateFloat&& tan(AccurateFloat&& a)
   {
      mpfr_tan(&a, &a, MPFR_RNDN);
      return std::move(a);
   }

   friend
   void sin_cos(AccurateFloat& s, AccurateFloat& c, const AccurateFloat& a)
   {
      mpfr_sin_cos(&s, &c, &a, MPFR_RNDN);
   }

   // basic arc functions

   friend
   AccurateFloat asin(const AccurateFloat& a)
   {
      AccurateFloat result;
      mpfr_asin(&result, &a, MPFR_RNDN);
      return result;
   }

   friend
   AccurateFloat&& asin(AccurateFloat&& a)
   {
      mpfr_asin(&a, &a, MPFR_RNDN);
      return std::move(a);
   }

   friend
   AccurateFloat acos(const AccurateFloat& a)
   {
      AccurateFloat result;
      mpfr_acos(&result, &a, MPFR_RNDN);
      return result;
   }

   friend
   AccurateFloat&& acos(AccurateFloat&& a)
   {
      mpfr_acos(&a, &a, MPFR_RNDN);
      return std::move(a);
   }

   friend
   AccurateFloat atan(const AccurateFloat& a)
   {
      AccurateFloat result;
      mpfr_atan(&result, &a, MPFR_RNDN);
      return result;
   }

   friend
   AccurateFloat&& atan(AccurateFloat&& a)
   {
      mpfr_atan(&a, &a, MPFR_RNDN);
      return std::move(a);
   }

   // basic hyperbolic trigonometric functions

   friend
   AccurateFloat sinh(const AccurateFloat& a)
   {
      AccurateFloat result;
      mpfr_sinh(&result, &a, MPFR_RNDN);
      return result;
   }

   friend
   AccurateFloat&& sinh(AccurateFloat&& a)
   {
      mpfr_sinh(&a, &a, MPFR_RNDN);
      return std::move(a);
   }

   friend
   AccurateFloat cosh(const AccurateFloat& a)
   {
      AccurateFloat result;
      mpfr_cosh(&result, &a, MPFR_RNDN);
      return result;
   }

   friend
   AccurateFloat&& cosh(AccurateFloat&& a)
   {
      mpfr_cosh(&a, &a, MPFR_RNDN);
      return std::move(a);
   }

   friend
   AccurateFloat tanh(const AccurateFloat& a)
   {
      AccurateFloat result;
      mpfr_tanh(&result, &a, MPFR_RNDN);
      return result;
   }

   friend
   AccurateFloat&& tanh(AccurateFloat&& a)
   {
      mpfr_tanh(&a, &a, MPFR_RNDN);
      return std::move(a);
   }

   friend
   void sinh_cosh(AccurateFloat& s, AccurateFloat& c, const AccurateFloat& a)
   {
      mpfr_sinh_cosh(&s, &c, &a, MPFR_RNDN);
   }

   // basic hyperbolic arc functions

   friend
   AccurateFloat asinh(const AccurateFloat& a)
   {
      AccurateFloat result;
      mpfr_asinh(&result, &a, MPFR_RNDN);
      return result;
   }

   friend
   AccurateFloat&& asinh(AccurateFloat&& a)
   {
      mpfr_asinh(&a, &a, MPFR_RNDN);
      return std::move(a);
   }

   friend
   AccurateFloat acosh(const AccurateFloat& a)
   {
      AccurateFloat result;
      mpfr_acosh(&result, &a, MPFR_RNDN);
      return result;
   }

   friend
   AccurateFloat&& acosh(AccurateFloat&& a)
   {
      mpfr_acosh(&a, &a, MPFR_RNDN);
      return std::move(a);
   }

   friend
   AccurateFloat atanh(const AccurateFloat& a)
   {
      AccurateFloat result;
      mpfr_atanh(&result, &a, MPFR_RNDN);
      return result;
   }

   friend
   AccurateFloat&& atanh(AccurateFloat&& a)
   {
      mpfr_atanh(&a, &a, MPFR_RNDN);
      return std::move(a);
   }

   Int compare(const AccurateFloat& b) const noexcept
   {
      return mpfr_cmp(this, &b);
   }

   Int compare(const Integer& b) const noexcept
   {
      const Int s1 = isinf(*this), s2 = isinf(b);
      if (__builtin_expect(s1 || s2, 0)) {
         return s1-s2;
      }
      return mpfr_cmp_z(this, &b);
   }

   Int compare(const Rational& b) const noexcept
   {
      const Int s1 = isinf(*this), s2 = isinf(b);
      if (__builtin_expect(s1 || s2, 0)) {
         return s1-s2;
      }
      return mpfr_cmp_q(this, &b);
   }

   Int compare(long b) const noexcept
   {
      return mpfr_cmp_si(this, b);
   }

   Int compare(int b) const noexcept
   {
      return compare(long(b));
   }

   Int compare(double b) const noexcept
   {
      return mpfr_cmp_d(this, b);
   }

   typedef mlist<int, long, double, Integer, Rational> is_comparable_with;

   bool is_zero() const noexcept
   {
      return mpfr_zero_p(this);
   }

   friend
   bool operator== (const AccurateFloat& a, const AccurateFloat& b)
   {
      return mpfr_equal_p(&a, &b);
   }

   friend
   bool operator!= (const AccurateFloat& a, const AccurateFloat& b)
   {
      return !(a==b);
   }

   friend
   bool operator< (const AccurateFloat& a, const AccurateFloat& b)
   {
      return mpfr_less_p(&a, &b);
   }

   friend
   bool operator<= (const AccurateFloat& a, const AccurateFloat& b)
   {
      return mpfr_lessequal_p(&a, &b);
   }

   friend
   bool operator> (const AccurateFloat& a, const AccurateFloat& b)
   {
      return b<a;
   }

   friend
   bool operator>= (const AccurateFloat& a, const AccurateFloat& b)
   {
      return b<=a;
   }

   template <typename T>
   friend
   typename std::enable_if<mlist_contains<is_comparable_with, T>::value, bool>::type
   operator== (const AccurateFloat& a, const T& b) noexcept
   {
      return a.compare(b)==0;
   }

   template <typename T>
   friend
   typename std::enable_if<mlist_contains<is_comparable_with, T>::value, bool>::type
   operator== (const T& a, const AccurateFloat& b) noexcept
   {
      return b==a;
   }

   template <typename T>
   friend
   typename std::enable_if<mlist_contains<is_comparable_with, T>::value, bool>::type
   operator!= (const AccurateFloat& a, const T& b) noexcept
   {
      return !(a==b);
   }

   template <typename T>
   friend
   typename std::enable_if<mlist_contains<is_comparable_with, T>::value, bool>::type
   operator!= (const T& a, const AccurateFloat& b) noexcept
   {
      return !(b==a);
   }

   template <typename T>
   friend
   typename std::enable_if<mlist_contains<is_comparable_with, T>::value, bool>::type
   operator< (const AccurateFloat& a, const T& b) noexcept
   {
      return a.compare(b)<0;
   }

   template <typename T>
   friend
   typename std::enable_if<mlist_contains<is_comparable_with, T>::value, bool>::type
   operator< (const T& a, const AccurateFloat& b) noexcept
   {
      return b>a;
   }

   template <typename T>
   friend
   typename std::enable_if<mlist_contains<is_comparable_with, T>::value, bool>::type
   operator> (const AccurateFloat& a, const T& b) noexcept
   {
      return a.compare(b)>0;
   }

   template <typename T>
   friend
   typename std::enable_if<mlist_contains<is_comparable_with, T>::value, bool>::type
   operator> (const T& a, const AccurateFloat& b) noexcept
   {
      return b<a;
   }

   template <typename T>
   friend
   typename std::enable_if<mlist_contains<is_comparable_with, T>::value, bool>::type
   operator<= (const AccurateFloat& a, const T& b) noexcept
   {
      return a.compare(b)<=0;
   }

   template <typename T>
   friend
   typename std::enable_if<mlist_contains<is_comparable_with, T>::value, bool>::type
   operator<= (const T& a, const AccurateFloat& b) noexcept
   {
      return b>=a;
   }

   template <typename T>
   friend
   typename std::enable_if<mlist_contains<is_comparable_with, T>::value, bool>::type
   operator>= (const AccurateFloat& a, const T& b) noexcept
   {
      return a.compare(b)>=0;
   }

   template <typename T>
   friend
   typename std::enable_if<mlist_contains<is_comparable_with, T>::value, bool>::type
   operator>= (const T& a, const AccurateFloat& b) noexcept
   {
      return b<=a;
   }

   friend
   bool abs_equal(const AccurateFloat& a, const AccurateFloat& b) noexcept
   {
      return !mpfr_cmpabs(&a, &b);
   }

   mpfr_srcptr get_rep() const { return this; }

   friend
   std::ostream& operator<< (std::ostream& os, const AccurateFloat& a)
   {
      a.putstr(os, os.flags());
      return os;
   }

   void read(std::istream& is);

   friend
   std::istream& operator>> (std::istream& is, AccurateFloat& a)
   {
      a.read(is);
      return is;
   }


#if POLYMAKE_DEBUG
   void dump() const __attribute__((used)) { std::cerr << *this << std::flush; }
#endif
protected:
   // +=,-= Rational, Integer
   bool add_inf(Int s2)
   {
      if (__builtin_expect(s2, 0)) {
         const Int s1 = isinf(*this);
         if (s1) {
            if (s2 != s1) mpfr_set_nan(this);
         } else {
            mpfr_set_inf(this, int(s2));
         }
         return true;
      }
      return false;
   }

   // +,- Rational, Integer
   bool init_add_inf(const AccurateFloat& a, Int s2)
   {
      if (__builtin_expect(s2, 0)) {
         const Int s1 = mpfr_inf_p(&a) ? mpfr_sgn(&a) : 0;
         if (!s1 || s2 == s1)
            mpfr_set_inf(this, int(s2));  // else *this remains NaN
         return true;
      }
      return false;
   }

   // *= Rational, Integer
   bool mul_inf(Int s2)
   {
      if (__builtin_expect(s2, 0)) {
         if (mpfr_zero_p(this))
            mpfr_set_nan(this);
         else if (!mpfr_nan_p(this))
            mpfr_set_inf(this, int(mpfr_sgn(this)*s2));
         return true;
      }
      return false;
   }

   // * Rational, Integer
   bool init_mul_inf(const AccurateFloat& a, Int s2)
   {
      if (__builtin_expect(s2, 0)) {
         if (!mpfr_zero_p(&a) && !mpfr_nan_p(&a))
            mpfr_set_inf(this, int(mpfr_sgn(&a)*s2));
         return true;
      }
      return false;
   }

   // /= Rational, Integer
   bool div_inf(Int s2)
   {
      if (__builtin_expect(s2, 0)) {
         if (mpfr_inf_p(this))
            mpfr_set_nan(this);
         else if (!mpfr_nan_p(this))
            mpfr_set_zero(this, int(mpfr_sgn(this)*s2));
         return true;
      }
      return false;
   }

   // / Rational, Integer
   bool init_div_inf(const AccurateFloat& a, Int s2)
   {
      if (__builtin_expect(s2, 0)) {
         if (!mpfr_inf_p(&a) && !mpfr_nan_p(&a))
            mpfr_set_zero(this, int(mpfr_sgn(&a)*s2));
         return true;
      }
      return false;
   }

   void putstr(std::ostream& os, std::ios::fmtflags) const;

   friend class Integer;
   friend class Rational;
};

template <>
struct spec_object_traits<AccurateFloat>
   : spec_object_traits<is_scalar> {

   static
   bool is_zero(const AccurateFloat& a)
   {
      return a.is_zero();
   }

   static
   bool is_one(const AccurateFloat& a)
   {
      return a==1;
   }

   static const AccurateFloat& zero();
   static const AccurateFloat& one();
};

inline
bool isfinite(const AccurateFloat& a) noexcept
{
   return mpfr_number_p(a.get_rep());
}

inline
Int isinf(const AccurateFloat& a) noexcept
{
   return mpfr_inf_p(a.get_rep()) ? mpfr_sgn(a.get_rep()) : 0;
}

inline
Int sign(const AccurateFloat& a) noexcept
{
   return sign(mpfr_sgn(a.get_rep()));
}

template <>
struct algebraic_traits<AccurateFloat> {
   typedef AccurateFloat ring_type;
   typedef AccurateFloat field_type;
};

AccurateFloat pow(const AccurateFloat& base, long exp);

}
namespace std {

inline
void swap(pm::AccurateFloat& a, pm::AccurateFloat& b)
{
   a.swap(b);
}

template <>
class numeric_limits<pm::AccurateFloat> : public numeric_limits<pm::Rational> {
public:
   static const bool is_exact = false;
   static pm::AccurateFloat min() { return pm::AccurateFloat::infinity(-1); }
   static pm::AccurateFloat max() { return pm::AccurateFloat::infinity(1); }
   static pm::AccurateFloat infinity() { return max(); }
};

}
namespace pm {
namespace operations {

template <>
struct cmp_scalar<AccurateFloat, AccurateFloat, void> : cmp_GMP_based<AccurateFloat> {};

} }
namespace polymake {
using pm::AccurateFloat;
}


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
