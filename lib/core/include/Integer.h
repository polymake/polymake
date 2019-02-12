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

/** @file Integer.h
    @brief Implementation of pm::Integer class
*/

#ifndef POLYMAKE_INTEGER_H
#define POLYMAKE_INTEGER_H

#include "polymake/internal/operations.h"
#include "polymake/internal/comparators.h"
#include "polymake/internal/converters.h"
#include "polymake/numerical_functions.h"
#include "polymake/internal/CharBuffer.h"

#include <gmp.h>
#include <limits>
#include <climits>
#include <stdexcept>
#include <cassert>

namespace pm {

/** @namespace GMP
    @brief Wrapper classes for @ref GMP "GMP"'s number types.
*/

namespace GMP {

/** @brief Exception type
    A constructor of Integer or Rational from const char* throws an exception
    of this type in case of a syntax error.
*/
class error : public std::domain_error {
public:
   explicit error(const std::string& what_arg)
      : std::domain_error(what_arg) {}
};

/// Exception type: "not a number".
class NaN : public error {
public:
   NaN();
};

/// Exception type: "division by zero".
class ZeroDivide : public error {
public:
   ZeroDivide();
};

/// Exception type: a number can't be casted to a smaller type without overflow or lost of data (e.g. non-integral Rational to Integer)
class BadCast : public error {
public:
   BadCast();

   BadCast(const std::string& what_arg)
      : error(what_arg) {}
};

}

// forward declarations needed for friend and specializations

class Integer; class Rational; class AccurateFloat; class Bitset;

template <> struct spec_object_traits<Integer>;

}

namespace std {

template <>
class numeric_limits<pm::Integer>;

template <>
class numeric_limits<pm::Rational>;

template <>
class numeric_limits<pm::AccurateFloat>;

}

namespace pm {

Integer gcd(const Integer& a, const Integer& b);
Integer&& gcd(Integer&& a, const Integer& b);
Integer gcd(const Integer& a, long b);
Integer&& gcd(Integer&& a, long b);

Integer lcm(const Integer& a, const Integer& b);
Integer&& lcm(Integer&& a, const Integer& b);
Integer lcm(const Integer& a, long b);
Integer&& lcm(Integer&& a, long b);

ExtGCD<Integer> ext_gcd(const Integer& a, const Integer& b);

Div<Integer> div(const Integer& a, const Integer& b);
Div<Integer> div(const Integer& a, long b);

Integer div_exact(const Integer& a, const Integer& b);
Integer div_exact(const Integer& a, long b);

/// data from third parties can't have infinite values
constexpr bool isfinite(const __mpz_struct&) { return true; }
constexpr int isinf(const __mpz_struct&) { return 0; }

bool isfinite(const Integer& a) noexcept;
int isinf(const Integer& a) noexcept;
int sign(const Integer& a) noexcept;

/** @class Integer
    @brief Integral number of unlimited precision

    Powered by GMP.
 */

class Integer
   : protected __mpz_struct {

   // flags for set_inf() and similar internal routines
   enum class initialized : bool { no, yes };
public:
   ~Integer() noexcept
   {
      if (_mp_d) mpz_clear(this);
   }

   /// Constructors

   Integer(const Integer& b)
   {
      set_data(b, initialized::no);
   }

   Integer(Integer&& b) noexcept
   {
      set_data(b, initialized::no);
   }

   /// Copy the value from a third party
   explicit Integer(const mpz_t& b)
   {
      set_data(b[0], initialized::no);
   }

   /// Steal the value from a third party
   /// The source must be re-initialized if it's going to be used afterwards
   explicit Integer(mpz_t&& b) noexcept
   {
      set_data(b[0], initialized::no);
   }

   Integer(long b=0)
   {
      mpz_init_set_si(this, b);
   }

   Integer(int b)
      : Integer(long(b)) {}


   /// these three are solely for libnormaliz
   Integer(unsigned long b)
   {
      mpz_init_set_ui(this, b);
   }

   Integer(unsigned int b)
      : Integer((unsigned long)b) {}

   Integer(long long b)
   {
      if (sizeof(long long)==sizeof(long) || (b >= std::numeric_limits<long>::min() && b <= std::numeric_limits<long>::max())) {
         mpz_init_set_si(this, long(b));
      } else {
         mpz_init2(this, sizeof(long long)*8);
         *this=b;
      }
   }

   /// conversion
   explicit Integer(double b)
   {
      set_data(b, initialized::no);
   }

   /// Construct as a copy of the numerator;
   /// @a b must be integral, otherwise GMP::BadCast exception will be raised
   explicit Integer(const Rational& b);

   explicit Integer(Rational&& b);

   explicit Integer(const AccurateFloat& b)
   {
      mpz_init(this);
      *this=b;
   }

   /// Recognizes automatically number base 10, 8, or 16, as well as singular values "±inf" and "nan".
   explicit Integer(const char* s);

   struct Reserve {};

   /// Reserve space for n bits.
   Integer(size_t n, Reserve)
   {
      mpz_init2(this, n);
   }

   /// Fill with a prescribed number of random bits
   Integer(gmp_randstate_t rnd, unsigned long bits)
   {
      mpz_init(this);
      mpz_urandomb(this, rnd, bits);
   }

   /// Construct a random number between 0 and @a upper
   Integer(gmp_randstate_t rnd, const Integer& upper)
   {
      mpz_init(this);
      mpz_urandomm(this, rnd, &upper);
   }

   /// Construct an infinite value with a given sign
   static
   Integer infinity(int sgn) noexcept
   {
      assert(sgn==-1 || sgn==1);
      Integer result(nullptr);
      set_inf(&result, sgn, initialized::no);
      return result;
   }

#if defined(__GMP_PLUSPLUS__)
   /// convert to the GMP's own C++ wrapper
   mpz_class gmp() const
   {
      return mpz_class(this);
   }
   
   /// construct an Integer from the GMP's own C++ wrapper
   Integer(const mpz_class& i)
   {
      mpz_init_set(this, i.get_mpz_t());
   }
#endif

   /// Assignment
   Integer& operator= (const Integer& b)
   {
      set_data(b, initialized::yes);
      return *this;
   }

   Integer& operator= (Integer&& b) noexcept
   {
      set_data(b, initialized::yes);
      return *this;
   }

   /// Assignment with conversion.
   Integer& operator= (long b)
   {
      set_data(b, initialized::yes);
      return *this;
   }

   Integer& operator= (int b)
   {
      return operator=(long(b));
   }

   /// these three are for libnormaliz again
   Integer& operator= (unsigned long b)
   {
      if (__builtin_expect(isfinite(*this), 1))
         mpz_set_ui(this, b);
      else
         mpz_init_set_ui(this, b);
      return *this;
   }

   Integer& operator= (unsigned int b)
   {
      return operator=(static_cast<unsigned long>(b));
   }

   Integer& operator= (long long b);

   /// Assignment with conversion.
   Integer& operator= (double b)
   {
      set_data(b, initialized::yes);
      return *this;
   }

   /// Assignment will fail if @a b is not integral
   Integer& operator= (const Rational& b);

   Integer& operator= (Rational&& b);

   Integer& operator= (const AccurateFloat&);

   /// Assign a copy of data obtained from a third party
   Integer& copy_from(mpz_srcptr src)
   {
      set_data(*src, initialized::yes);
      return *this;
   }

   /// Recognizes automatically number base 10, 8, or 16, as well as special values "±inf".
   Integer& set(const char *s)
   {
      if (__builtin_expect(!isfinite(*this), 0))
         mpz_init(this);
      parse(s);
      return *this;
   }

   /// Fill with bits coming from an open file.
   /// The current allocated size is preserved (see constructor with Reserve() argument).
   /// @retval true if filled successfully, false if eof or read error occured.
   bool fill_from_file(int fd);

   /// Efficiently swapping two Integer objects.
   void swap(Integer& b) noexcept
   {
      mpz_swap(this, &b);
   }

   /// TODO: kill this
   friend void relocate(Integer* from, Integer* to)
   {
      static_cast<__mpz_struct&>(*to)=*from;
   }

   /// Cast to simpler types

   explicit operator double() const
   {
      const int is_inf=isinf(*this);
      if (__builtin_expect(is_inf, 0))
         return is_inf * std::numeric_limits<double>::infinity();
      return mpz_get_d(this);
   }

   explicit operator long() const
   {
      if (isfinite(*this) && mpz_fits_slong_p(this))
         return mpz_get_si(this);
      throw GMP::BadCast();
   }

   explicit operator int() const
   {
      if (isfinite(*this) && mpz_fits_sint_p(this))
         return mpz_get_si(this);
      throw GMP::BadCast();
   }

   explicit operator long long() const;

   /// Unary operators

   friend
   Integer operator+ (const Integer& a)
   {
      return a;
   }

   friend
   Integer&& operator+ (Integer&& a)
   {
      return std::move(a);
   }

   /// Increment.
   Integer& operator++()
   {
      if (__builtin_expect(isfinite(*this), 1)) mpz_add_ui(this, this, 1);
      return *this;
   }

   Integer operator++ (int) { Integer copy(*this); operator++(); return copy; }

   /// Decrement.
   Integer& operator--()
   {
      if (__builtin_expect(isfinite(*this), 1)) mpz_sub_ui(this, this, 1);
      return *this;
   }

   Integer operator-- (int) { Integer copy(*this); operator--(); return copy; }

   /// In-place negation.
   Integer& negate() noexcept
   {
      _mp_size=-_mp_size;
      return *this;
   }

   /// Negation
   friend
   Integer operator- (const Integer& a)
   {
      Integer result(a);
      result.negate();
      return result;
   }

   friend
   Integer&& operator- (Integer&& a)
   {
      return std::move(a.negate());
   }

   /// Addition

   Integer& operator+= (const Integer& b)
   {
      if (__builtin_expect(isfinite(*this), 1)) {
         if (__builtin_expect(isfinite(b), 1))
            mpz_add(this, this, &b);
         else
            set_inf(this, b);
      } else {
         if (isinf(*this)+isinf(b)==0)
            throw GMP::NaN();
      }
      return *this;
   }

   friend
   Integer operator+ (const Integer& a, const Integer& b)
   {
      Integer result;
      if (__builtin_expect(isfinite(a), 1)) {
         if (__builtin_expect(isfinite(b), 1))
            mpz_add(&result, &a, &b);
         else
            set_inf(&result, b);
      } else {
         if (isinf(a)+isinf(b)==0)
            throw GMP::NaN();
         set_inf(&result, a);
      }
      return result;
   }

   friend
   Integer&& operator+ (Integer&& a, const Integer& b)
   {
      return std::move(a+=b);
   }
   friend
   Integer&& operator+ (const Integer& a, Integer&& b)
   {
      return std::move(b+=a);
   }
   friend
   Integer&& operator+ (Integer&& a, Integer&& b)
   {
      return a._mp_alloc >= b._mp_alloc ? std::move(a+=b) : std::move(b+=a);
   }

   Integer& operator+= (long b)
   {
      if (__builtin_expect(isfinite(*this), 1)) {
         if (b>=0) {
            mpz_add_ui(this, this, b);
         } else {
            mpz_sub_ui(this, this, -b);
         }
      }
      return *this;
   }
   Integer& operator+= (int b)
   {
      return *this += long(b);
   }

   friend
   Integer operator+ (const Integer& a, long b)
   {
      Integer result(a);
      result += b;
      return result;
   }
   friend
   Integer&& operator+ (Integer&& a, long b)
   {
      return std::move(a+=b);
   }
   friend
   Integer operator+ (const Integer& a, int b)
   {
      return a+long(b);
   }
   friend
   Integer&& operator+ (Integer&& a, int b)
   {
      return std::move(a+=long(b));
   }
   friend
   Integer operator+ (long a, const Integer& b)
   {
      return b+a;
   }
   friend
   Integer operator+ (int a, const Integer& b)
   {
      return b+long(a);
   }
   friend
   Integer&& operator+ (long a, Integer&& b)
   {
      return std::move(b+=a);
   }
   friend
   Integer&& operator+ (int a, Integer&& b)
   {
      return std::move(b+=long(a));
   }

   /// Subtraction

   Integer& operator-= (const Integer& b)
   {
      if (__builtin_expect(isfinite(*this), 1)) {
         if (__builtin_expect(isfinite(b), 1))
            mpz_sub(this, this, &b);
         else
            set_inf(this, -1, b);
      } else {
         if (isinf(*this)==isinf(b))
            throw GMP::NaN();
      }
      return *this;
   }

   friend
   Integer operator- (const Integer& a, const Integer& b)
   {
      Integer result;
      if (__builtin_expect(isfinite(a), 1)) {
         if (__builtin_expect(isfinite(b), 1))
            mpz_sub(&result, &a, &b);
         else
            set_inf(&result, -1, b);
      } else {
         if (isinf(a)==isinf(b))
            throw GMP::NaN();
         set_inf(&result, a);
      }
      return result;
   }

   friend
   Integer&& operator- (Integer&& a, const Integer& b)
   {
      return std::move(a-=b);
   }
   friend
   Integer&& operator- (const Integer& a, Integer&& b)
   {
      return std::move((b-=a).negate());
   }
   friend
   Integer&& operator- (Integer&& a, Integer&& b)
   {
      return a._mp_alloc >= b._mp_alloc ? std::move(a-=b) : std::move((b-=a).negate());
   }

   Integer& operator-= (long b)
   {
      if (__builtin_expect(isfinite(*this),1)) {
         if (b>=0) {
            mpz_sub_ui(this, this, b);
         } else {
            mpz_add_ui(this, this, -b);
         }
      }
      return *this;
   }
   Integer& operator-= (int b)
   {
      return *this -= long(b);
   }

   friend
   Integer operator- (const Integer& a, long b)
   {
      Integer result(a);
      result-=b;
      return result;
   }
   friend
   Integer operator- (const Integer& a, int b)
   {
      return a-long(b);
   }
   friend
   Integer&& operator- (Integer&& a, long b)
   {
      return std::move(a-=b);
   }
   friend
   Integer&& operator- (Integer&& a, int b)
   {
      return std::move(a-=long(b));
   }

   friend
   Integer operator- (long a, const Integer& b)
   {
      Integer result(b-a);
      result.negate();
      return result;
   }
   friend
   Integer operator- (int a, const Integer& b)
   {
      return long(a)-b;
   }
   friend
   Integer&& operator- (long a, Integer&& b)
   {
      return std::move((b-=a).negate());
   }
   friend
   Integer&& operator- (int a, Integer&& b)
   {
      return std::move((b-=long(a)).negate());
   }

   /// Multiplication

   Integer& operator*= (const Integer& b)
   {
      if (__builtin_expect(isfinite(*this), 1)) {
         if (__builtin_expect(isfinite(b), 1))
            mpz_mul(this, this, &b);
         else
            set_inf(this, mpz_sgn(this), b);
      } else {
         inf_inv_sign(this, mpz_sgn(&b));
      }
      return *this;
   }

   friend
   Integer operator* (const Integer& a, const Integer& b)
   {
      Integer result;
      if (__builtin_expect(isfinite(a), 1)) {
         if (__builtin_expect(isfinite(b), 1))
            mpz_mul(&result, &a, &b);
         else
            set_inf(&result, mpz_sgn(&a), b);
      } else {
         set_inf(&result, mpz_sgn(&b), a);
      }
      return result;
   }

   friend
   Integer&& operator* (Integer&& a, const Integer& b)
   {
      return std::move(a*=b);
   }
   friend
   Integer&& operator* (const Integer& a, Integer&& b)
   {
      return std::move(b*=a);
   }
   friend
   Integer&& operator* (Integer&& a, Integer&& b)
   {
      return std::move(a*=b);
   }

   Integer& operator*= (long b)
   {
      if (__builtin_expect(isfinite(*this), 1))
         mpz_mul_si(this, this, b);
      else
         inf_inv_sign(this, b);
      return *this;
   }
   Integer& operator*= (int b)
   {
      return *this *= long(b);
   }

   friend
   Integer operator* (const Integer& a, long b)
   {
      Integer result(a);
      result*=b;
      return result;
   }
   friend
   Integer operator* (const Integer& a, int b)
   {
      return a*long(b);
   }
   friend
   Integer&& operator* (Integer&& a, long b)
   {
      return std::move(a*=b);
   }
   friend
   Integer&& operator* (Integer&& a, int b)
   {
      return std::move(a*=long(b));
   }
   friend
   Integer operator* (long a, const Integer& b)
   {
      return b*a;
   }
   friend
   Integer operator* (int a, const Integer& b)
   {
      return b*long(a);
   }
   friend
   Integer&& operator* (long a, Integer&& b)
   {
      return std::move(b*=a);
   }
   friend
   Integer&& operator* (int a, Integer&& b)
   {
      return std::move(b*=long(a));
   }

   /// Division with rounding via truncation

   Integer& operator/= (const Integer& b)
   {
      if (__builtin_expect(isfinite(*this), 1)) {
         if (__builtin_expect(isfinite(b), 1)) {
            if (__builtin_expect(!b.is_zero(), 1))
               mpz_tdiv_q(this, this, &b);
            else
               throw GMP::ZeroDivide();
         } else {
            mpz_set_ui(this, 0);
         }
      } else {
         if (!isfinite(b))
            throw GMP::NaN();
         inf_inv_sign(this, mpz_sgn(&b));
      }
      return *this;
   }

   friend
   Integer operator/ (const Integer& a, const Integer& b)
   {
      Integer result(a);
      result /= b;
      return result;
   }
   friend
   Integer&& operator/ (Integer&& a, const Integer& b)
   {
      return std::move(a/=b);
   }

   Integer& operator/= (long b)
   {
      if (__builtin_expect(isfinite(*this), 1)) {
         if (__builtin_expect(b, 1)) {
            if (b>0) {
               mpz_tdiv_q_ui(this, this, b);
            } else {
               mpz_tdiv_q_ui(this, this, -b);
               negate();
            }
         } else {
            throw GMP::ZeroDivide();
         }
      } else {
         inf_inv_sign(this, b);
      }
      return *this;
   }

   Integer& operator/= (int b)
   {
      return *this /= long(b);
   }

   friend
   Integer operator/ (const Integer& a, long b)
   {
      Integer result(a);
      result/=b;
      return result;
   }
   friend
   Integer operator/ (const Integer& a, int b)
   {
      return a/long(b);
   }
   friend
   Integer&& operator/ (Integer&& a, long b)
   {
      return std::move(a/=b);
   }
   friend
   Integer&& operator/ (Integer&& a, int b)
   {
      return std::move(a/=long(b));
   }

   friend
   long operator/ (long a, const Integer& b)
   {
      if (__builtin_expect(isfinite(b), 1)) {
         if (__builtin_expect(!b.is_zero(), 1)) {
            if (mpz_fits_slong_p(&b))
               return a/mpz_get_si(&b);
         } else {
            throw GMP::ZeroDivide();
         }
      }
      return 0;
   }

   friend
   long operator/ (int a, const Integer& b)
   {
      return long(a)/b;
   }

   /// Remainder of division.

   Integer& operator%= (const Integer& b)
   {
      if (__builtin_expect(isfinite(*this) && isfinite(b), 1)) {
         if (__builtin_expect(!b.is_zero(), 1))
            mpz_tdiv_r(this, this, &b);
         else
            throw GMP::ZeroDivide();
      } else {
         throw GMP::NaN();
      }
      return *this;
   }

   friend
   Integer operator% (const Integer& a, const Integer& b)
   {
      Integer result(a);
      result %= b;
      return result;
   }
   friend
   Integer&& operator% (Integer&& a, const Integer& b)
   {
      return std::move(a%=b);
   }

   Integer& operator%= (long b)
   {
      if (__builtin_expect(isfinite(*this), 1)) {
         if (__builtin_expect(b, 1))
            mpz_tdiv_r_ui(this, this, std::abs(b));
         else
            throw GMP::ZeroDivide();
      } else {
         throw GMP::NaN();
      }
      return *this;
   }
   Integer& operator%= (int b)
   {
      return *this %= long(b);
   }

   friend
   long operator% (const Integer& a, long b)
   {
      if (__builtin_expect(isfinite(a), 1)) {
         if (__builtin_expect(b, 1))
            return mpz_tdiv_ui(&a, std::abs(b));
         else
            throw GMP::ZeroDivide();
      } else {
         throw GMP::NaN();
      }
   }
   friend
   int operator% (const Integer& a, int b)
   {
      return a % long(b);
   }

   // these two just suppress a warning about "ambiguous candidates"
   friend
   long operator% (Integer&& a, long b)
   {
      return const_cast<const Integer&>(a) % b;
   }
   friend
   int operator% (Integer&& a, int b)
   {
      return const_cast<const Integer&>(a) % long(b);
   }

   friend
   long operator% (long a, const Integer& b)
   {
      if (__builtin_expect(isfinite(b), 1)) {
         if (__builtin_expect(!b.is_zero(), 1)) {
            if (mpz_fits_slong_p(&b))
               return a % mpz_get_si(&b);
            else
               return a;
         } else {
            throw GMP::ZeroDivide();
         }
      } else {
         throw GMP::NaN();
      }
   }

   friend
   int operator% (int a, const Integer& b)
   {
      return long(a) % b;
   }

   /// b != infinity; but 0/0 allowed
   Integer& div_exact(const Integer& b)
   {
      if (__builtin_expect(isfinite(*this), 1)) {
         if (__builtin_expect(!b.is_zero(), 1))
            mpz_divexact(this, this, &b);
      } else {
         inf_inv_sign(this, mpz_sgn(&b));
      }
      return *this;
   }

   /// 0/0 allowed
   Integer& div_exact(long b)
   {
      if (__builtin_expect(isfinite(*this), 1)) {
         if (__builtin_expect(b, 1)) {
            if (b>0) {
               mpz_divexact_ui(this, this, b);
            } else {
               mpz_divexact_ui(this, this, -b);
               negate();
            }
         }
      } else {
         inf_inv_sign(this, b);
      }
      return *this;
   }

   Integer& div_exact(int b)
   {
      return div_exact(long(b));
   }

   friend
   Integer div_exact(const Integer& a, const Integer& b);

   friend
   Integer div_exact(const Integer& a, long b);


   /// Obtain both the quotient and remainder of a division
   friend
   Div<Integer> div(const Integer& a, const Integer& b);

   friend
   Div<Integer> div(const Integer& a, long b);


   /// Multiply by or divide through 2**k, truncate to zero.
   Integer& operator<<= (long k)
   {
      if (__builtin_expect(isfinite(*this), 1)) {
         if (k >= 0)
            mpz_mul_2exp(this, this, k);
         else
            mpz_tdiv_q_2exp(this, this, -k);
      }
      return *this;
   }
   Integer& operator<<= (int k)
   {
      return *this <<= long(k);
   }

   friend
   Integer operator<< (const Integer& a, long k)
   {
      Integer result(a);
      result <<= k;
      return result;
   }
   friend
   Integer&& operator<< (Integer&& a, long k)
   {
      return std::move(a <<= k);
   }
   friend
   Integer operator<< (const Integer& a, int k)
   {
      return a << long(k);
   }
   friend
   Integer&& operator<< (Integer&& a, int k)
   {
      return std::move(a <<= long(k));
   }

   /// Divide through or multiply by 2**k, truncate to zero.
   Integer& operator>>= (long k)
   {
      return *this <<= -k;
   }
   Integer& operator>>= (int k)
   {
      return *this >>= long(k);
   }

   friend
   Integer operator>> (const Integer& a, long k)
   {
      Integer result(a);
      result >>= k;
      return result;
   }
   friend
   Integer&& operator>> (Integer&& a, long k)
   {
      return std::move(a >>= k);
   }
   friend
   Integer operator>> (const Integer& a, int k)
   {
      return a >> long(k);
   }
   friend
   Integer&& operator>> (Integer&& a, int k)
   {
      return std::move(a >>= long(k));
   }


   /// Test for bits.
   bool bit(unsigned long i) const
   {
      return isfinite(*this) && mpz_tstbit(this, i);
   }

   /// Parity.
   bool odd() const
   {
      return bit(0);
   }

   /// Parity.
   bool even() const
   {
      return isfinite(*this) && !odd();
   }


   /// fast comparison with 0
   bool is_zero() const noexcept
   {
      return mpz_sgn(this)==0;
   }

   /// Comparison. The magnitude of the return value is arbitrary, only its sign is relevant.
   int compare(const Integer& b) const
   {
      if (__builtin_expect(isfinite(*this) && isfinite(b), 1))
         return mpz_cmp(this, &b);
      else
         return isinf(*this)-isinf(b);
   }

   int compare(long b) const
   {
      if (__builtin_expect(isfinite(*this), 1))
         return mpz_cmp_si(this, b);
      else
         return isinf(*this);
   }

   int compare(int b) const
   {
      return compare(long(b));
   }

   int compare(double b) const
   {
      if (__builtin_expect(isfinite(*this) && isfinite(b), 1))
         return mpz_cmp_d(this, b);
      else
         return isinf(*this)-isinf(b);
   }

   typedef mlist<int, long, double> is_comparable_with;
   typedef mlist_concat<Integer, is_comparable_with>::type self_and_comparable_with;

   template <typename T, typename=typename std::enable_if<mlist_contains<self_and_comparable_with, T>::value>::type>
   friend
   bool operator== (const Integer& a, const T& b)
   {
      return !a.compare(b);
   }
   template <typename T, typename=typename std::enable_if<mlist_contains<self_and_comparable_with, T>::value>::type>
   friend
   bool operator!= (const Integer& a, const T& b)
   {
      return !(a==b);
   }
   template <typename T, typename=typename std::enable_if<mlist_contains<self_and_comparable_with, T>::value>::type>
   friend
   bool operator< (const Integer& a, const T& b)
   {
      return a.compare(b)<0;
   }
   template <typename T, typename=typename std::enable_if<mlist_contains<self_and_comparable_with, T>::value>::type>
   friend
   bool operator> (const Integer& a, const T& b)
   {
      return a.compare(b)>0;
   }
   template <typename T, typename=typename std::enable_if<mlist_contains<self_and_comparable_with, T>::value>::type>
   friend
   bool operator<= (const Integer& a, const T& b)
   {
      return a.compare(b)<=0;
   }
   template <typename T, typename=typename std::enable_if<mlist_contains<self_and_comparable_with, T>::value>::type>
   friend
   bool operator>= (const Integer& a, const T& b)
   {
      return a.compare(b)>=0;
   }
   template <typename T, typename=typename std::enable_if<mlist_contains<is_comparable_with, T>::value>::type>
   friend
   bool operator== (const T& a, const Integer& b)
   {
      return b==a;
   }
   template <typename T, typename=typename std::enable_if<mlist_contains<is_comparable_with, T>::value>::type>
   friend
   bool operator!= (const T& a, const Integer& b)
   {
      return !(b==a);
   }
   template <typename T, typename=typename std::enable_if<mlist_contains<is_comparable_with, T>::value>::type>
   friend
   bool operator< (const T& a, const Integer& b)
   {
      return b>a;
   }
   template <typename T, typename=typename std::enable_if<mlist_contains<is_comparable_with, T>::value>::type>
   friend
   bool operator> (const T& a, const Integer& b)
   {
      return b<a;
   }
   template <typename T, typename=typename std::enable_if<mlist_contains<is_comparable_with, T>::value>::type>
   friend
   bool operator<= (const T& a, const Integer& b)
   {
      return b>=a;
   }
   template <typename T, typename=typename std::enable_if<mlist_contains<is_comparable_with, T>::value>::type>
   friend
   bool operator>= (const T& a, const Integer& b)
   {
      return b<=a;
   }

   /// Equality of absolute values.
   friend
   bool abs_equal(const Integer& a, const Integer& b)
   {
      if (__builtin_expect(isfinite(a) && isfinite(b), 1))
         return !mpz_cmpabs(&a, &b);
      else
         return isinf(a) && isinf(b);
   }

   /// Equality of absolute values.
   friend
   bool abs_equal(const Integer& a, long b)
   {
      return __builtin_expect(isfinite(a), 1) && !mpz_cmpabs_ui(&a, std::abs(b));
   }

   friend
   bool abs_equal(const Integer& a, int b)
   {
      return abs_equal(a, long(b));
   }

   /// Equality of absolute values.
   friend
   bool abs_equal(const Integer& a, double b)
   {
      if (__builtin_expect(isfinite(a), 1))
         return !mpz_cmpabs_d(&a, b);
      return isinf(b);
   }

   template <typename T, typename=typename std::enable_if<mlist_contains<is_comparable_with, T>::value>::type>
   friend
   bool abs_equal(const T& a, const Integer& b)
   {
      return abs_equal(b, a);
   }


   /// Factorial.
   static
   Integer fac(long k)
   {
      if (k<0) throw GMP::NaN();
      Integer result;
      mpz_fac_ui(&result, k);
      return result;
   }

   /// Power.
   static
   Integer pow(const Integer& a, long k)
   {
      if (__builtin_expect(k < 0, 0))
         throw GMP::NaN();
      Integer result;
      if (__builtin_expect(isfinite(a), 1))
         mpz_pow_ui(&result, &a, k);
      else if (k)
         set_inf(&result, k%2 ? mpz_sgn(&a) : 1);
      else
         throw GMP::NaN();
      return result;
   }

   static
   Integer pow(long a, long k)
   {
      if (__builtin_expect(k < 0, 0))
         throw GMP::NaN();
      Integer result;
      if (a>=0) {
         mpz_ui_pow_ui(&result, a, k);
      } else {
         mpz_ui_pow_ui(&result, -a, k);
         result.negate();
      }
      return result;
   }

   static
   Integer pow(int a, long k)
   {
      return pow(long(a), k);
   }

   /// Square Root.
   friend
   Integer sqrt(const Integer& a)
   {
      if (mpz_sgn(&a)<0)
         throw GMP::NaN();
      Integer result;
      if (__builtin_expect(isfinite(a), 1))
         mpz_sqrt(&result, &a);
      else
         set_inf(&result, 1);
      return result;
   }

   friend
   Integer&& sqrt(Integer&& a)
   {
      if (mpz_sgn(&a)<0)
         throw GMP::NaN();
      if (__builtin_expect(isfinite(a), 1))
         mpz_sqrt(&a, &a);
      return std::move(a);
   }

   /// Absolute value.
   friend
   Integer abs(const Integer& a)
   {
      Integer result(a);
      mpz_abs(&result, &result);
      return result;
   }

   friend
   Integer&& abs(Integer&& a)
   {
      mpz_abs(&a, &a);
      return std::move(a);
   }

   /// Greatest common divisor.
   friend
   Integer gcd(const Integer& a, const Integer& b);

   friend
   Integer&& gcd(Integer&& a, const Integer& b);

   /// Greatest common divisor.
   friend
   Integer gcd(const Integer& a, long b);

   friend
   Integer&& gcd(Integer&& a, long b);

   /// Least common multiple.
   friend
   Integer lcm(const Integer& a, const Integer& b);

   friend
   Integer&& lcm(Integer&& a, const Integer& b);

   friend
   Integer lcm(const Integer& a, long b);

   friend
   Integer&& lcm(Integer&& a, long b);

   /// Extended gcd algorithm: g=a*p+b*q, a=m*g, b=n*g.
   friend
   ExtGCD<Integer> ext_gcd(const Integer& a, const Integer& b);

   /// Binomial coefficient.
   static
   Integer binom(const Integer& n, long k);

   /// Binomial coefficient.
   static
   Integer binom(long n, long k);

   static
   Integer binom(int n, long k)
   {
      return binom(long(n), k);
   }

   /// @param allow_sign whether leading whitespaces and sign are expected
   void read(std::istream& is, bool allow_sign=true);

   /// Calculates the size of the buffer needed to store an ASCII representation of an Integer.
   size_t strsize(std::ios::fmtflags flags) const;

   /** Produces a printable representation of an Integer.
       @param buf buffer of size not less than the return value of strsize().
   */
   void putstr(std::ios::fmtflags flags, char *buf) const;

   mpz_srcptr get_rep() const noexcept { return this; }

   /// Output to stream.
   friend
   std::ostream& operator<< (std::ostream& os, const Integer& a)
   {
      const std::ios::fmtflags flags=os.flags();
      a.putstr(flags, OutCharBuffer::reserve(os, a.strsize(flags)));
      return os;
   }

   /// Input from stream.
   friend
   std::istream& operator>> (std::istream& is, Integer& a)
   {
      a.read(is);
      return is;
   }
#if POLYMAKE_DEBUG
   void dump() const __attribute__((used)) { std::cerr << *this << std::flush; }
#endif
protected:
   /// uninitialized object
   explicit Integer(std::nullptr_t) {}

   /// parse the string and assign the value.
   /// throws an exception in case of syntax errors
   void parse(const char *s);

   static
   void set_finite(mpz_ptr me, __mpz_struct& src, initialized st)
   {
      if (bool(st)) {
         // move during assignment
         mpz_swap(me, &src);
      } else {
         // move during construction
         *me=src;
         set_inf(&src, 0, st);
      }
   }

   static
   void set_finite(mpz_ptr me, const __mpz_struct& src, initialized st)
   {
      if (bool(st) && __builtin_expect(me->_mp_d != nullptr, 1))
         mpz_set(me, &src);
      else
         mpz_init_set(me, &src);
   }

   static
   void set_finite(mpz_ptr me, long src, initialized st)
   {
      if (bool(st) && __builtin_expect(me->_mp_d != nullptr, 1))
         mpz_set_si(me, src);
      else
         mpz_init_set_si(me, src);
   }

   static
   void set_finite(mpz_ptr me, int src, initialized st)
   {
      set_finite(me, long(src), st);
   }

   static
   void set_finite(mpz_ptr me, double src, initialized st)
   {
      if (bool(st) && __builtin_expect(me->_mp_d != nullptr, 1))
         mpz_set_d(me, src);
      else
         mpz_init_set_d(me, src);
   }

   static
   void set_inf(mpz_ptr me, int sign, initialized st=initialized::yes) noexcept
   {
      if (bool(st) && me->_mp_d) mpz_clear(me);
      me->_mp_alloc=0;
      me->_mp_size=sign;
      me->_mp_d=nullptr;
   }
   static
   void set_inf(mpz_ptr me, const Integer& from, initialized st=initialized::yes) noexcept
   {
      set_inf(me, from._mp_size, st);
   }
   static
   void set_inf(mpz_ptr me, int sign, long inv, initialized st=initialized::yes)
   {
      if (sign==0 || inv==0)
         throw GMP::NaN();
      set_inf(me, inv<0 ? -sign : sign, st);
   }
   static
   void set_inf(mpz_ptr me, int sign, const Integer& from, initialized st=initialized::yes)
   {
      set_inf(me, sign, from._mp_size, st);
   }
   static
   void inf_inv_sign(mpz_ptr me, long s)
   {
      if (s==0 || me->_mp_size==0)
         throw GMP::NaN();
      if (s<0)
         me->_mp_size = -me->_mp_size;
   }

   template <typename Src>
   void set_data(Src&& src, initialized st)
   {
      if (__builtin_expect(isfinite(src), 1))
         set_finite(this, src, st);
      else
         set_inf(this, isinf(src), st);
   }

   friend class Rational;
   friend class AccurateFloat;
   friend class Bitset;
   friend struct spec_object_traits<Integer>;
};

}
namespace std {

inline void swap(pm::Integer& i1, pm::Integer& i2) noexcept { i1.swap(i2); }

template <>
class numeric_limits<pm::Integer> : public numeric_limits<long> {
public:
   static const bool has_infinity=true;

   static pm::Integer min() noexcept { return pm::Integer::infinity(-1); }
   static pm::Integer max() noexcept { return pm::Integer::infinity(1); }
   static pm::Integer infinity() noexcept { return pm::Integer::infinity(1); }

   static const int digits=INT_MAX;
   static const int digits10=INT_MAX;
   static const bool is_bounded=false;
};

}
namespace pm {

inline
bool isfinite(const Integer& a) noexcept
{
   return a.get_rep()->_mp_alloc;
}

inline
int isinf(const Integer& a) noexcept
{
   return isfinite(a) ? 0 : a.get_rep()->_mp_size;
}

inline
int sign(const Integer& a) noexcept
{
   return mpz_sgn(a.get_rep());
}

inline
Integer gcd(const Integer& a, const Integer& b)
{
   const bool f1=isfinite(a), f2=isfinite(b);
   if (__builtin_expect(f1 && f2, 1)) {
      Integer result;
      mpz_gcd(&result, &a, &b);
      return result;
   }
   return f1 ? a : b;
}

inline
Integer&& gcd(Integer&& a, const Integer& b)
{
   const bool f1=isfinite(a), f2=isfinite(b);
   if (__builtin_expect(f1 && f2, 1)) {
      mpz_gcd(&a, &a, &b);
   } else if (f2) {
      a=b;
   }
   return std::move(a);
}

inline
Integer&& gcd(const Integer& a, Integer&& b)
{
   return gcd(std::move(b), a);
}

inline
Integer&& gcd(Integer&& a, Integer&& b)
{
   return gcd(std::move(a), b);
}

inline
Integer gcd(const Integer& a, long b)
{
   if (__builtin_expect(isfinite(a), 1)) {
      Integer result;
      mpz_gcd_ui(&result, &a, b >= 0 ? b : -b);
      return result;
   }
   return b;
}
inline
Integer gcd(const Integer& a, int b)
{
   return gcd(a, long(b));
}

inline
Integer&& gcd(Integer&& a, long b)
{
   if (__builtin_expect(isfinite(a), 1))
      mpz_gcd_ui(&a, &a, std::abs(b));
   else
      a=b;
   return std::move(a);
}
inline
Integer&& gcd(Integer&& a, int b)
{
   return gcd(std::move(a), long(b));
}

inline
Integer gcd(long a, const Integer& b)
{
   return gcd(b, a);
}
inline
Integer gcd(int a, const Integer& b)
{
   return gcd(b, long(a));
}
inline
Integer&& gcd(long a, Integer&& b)
{
   return gcd(std::move(b), a);
}
inline
Integer&& gcd(int a, Integer&& b)
{
   return gcd(std::move(b), long(a));
}

inline
Integer lcm(const Integer& a, const Integer& b)
{
   Integer result;
   if (__builtin_expect(isfinite(a) && isfinite(b), 1))
      mpz_lcm(&result, &a, &b);
   else
      Integer::set_inf(&result, 1);
   return result;
}

inline
Integer&& lcm(Integer&& a, const Integer& b)
{
   const bool f1=isfinite(a), f2=isfinite(b);
   if (__builtin_expect(f1 && f2, 1))
      mpz_lcm(&a, &a, &b);
   else if (f1)
      a=b;
   return std::move(a);
}

inline
Integer&& lcm(const Integer& a, Integer&& b)
{
   return lcm(std::move(b), a);
}

inline
Integer&& lcm(Integer&& a, Integer&& b)
{
   return lcm(std::move(a), b);
}

inline
Integer lcm(const Integer& a, long b)
{
   Integer result;
   if (__builtin_expect(isfinite(a), 1))
      mpz_lcm_ui(&result, &a, b >= 0 ? b : -b);
   else
      Integer::set_inf(&result, 1);
   return result;
}
inline
Integer lcm(const Integer& a, int b)
{
   return lcm(a, long(b));
}
inline
Integer&& lcm(Integer&& a, long b)
{
   if (__builtin_expect(isfinite(a), 1))
      mpz_lcm_ui(&a, &a, b >= 0 ? b : -b);
   return std::move(a);
}
inline
Integer&& lcm(Integer&& a, int b)
{
   return lcm(std::move(a), long(b));
}
inline
Integer lcm(long a, const Integer& b)
{
   return lcm(b, a);
}
inline
Integer lcm(int a, const Integer& b)
{
   return lcm(b, long(a));
}
inline
Integer lcm(long a, Integer&& b)
{
   return lcm(std::move(b), a);
}
inline
Integer lcm(int a, Integer&& b)
{
   return lcm(std::move(b), long(a));
}

inline
ExtGCD<Integer> ext_gcd(const Integer& a, const Integer& b)
{
   ExtGCD<Integer> res;
   const bool f1=isfinite(a), f2=isfinite(b);
   if (__builtin_expect(f1 && f2, 1)) {
      mpz_gcdext(&res.g, &res.p, &res.q, &a, &b);
      mpz_divexact(&res.k1, &a, &res.g);
      mpz_divexact(&res.k2, &b, &res.g);
   } else if (f1) {
      res.g=a; res.p=1; res.q=0; res.k1=1; res.k2=b;
   } else {
      res.g=b; res.p=0; res.q=1; res.k1=a; res.k2=1;
   }
   return res;
}

inline
Div<Integer> div(const Integer& a, const Integer& b)
{
   Div<Integer> res;
   if (__builtin_expect(isfinite(a) && isfinite(b), 1)) {
      if (__builtin_expect(!b.is_zero(), 1))
         mpz_tdiv_qr(&res.quot, &res.rem, &a, &b);
      else
         throw GMP::ZeroDivide();
   } else {
      throw GMP::NaN();
   }
   return res;
}

inline
Div<Integer> div(const Integer& a, long b)
{
   Div<Integer> res;
   if (__builtin_expect(isfinite(a), 1)) {
      if (__builtin_expect(b, 1))
         mpz_tdiv_qr_ui(&res.quot, &res.rem, &a, std::abs(b));
      else
         throw GMP::ZeroDivide();
   } else {
      throw GMP::NaN();
   }
   return res;
}

inline
Div<Integer> div(const Integer& a, int b)
{
   return div(a, long(b));
}

inline
Integer div_exact(const Integer& a, const Integer& b)
{
   Integer result(a);
   result.div_exact(b);
   return result;
}

inline
Integer&& div_exact(Integer&& a, const Integer& b)
{
   return std::move(a.div_exact(b));
}

inline
Integer div_exact(const Integer& a, long b)
{
   Integer result(a);
   result.div_exact(b);
   return result;
}
inline
Integer div_exact(const Integer& a, int b)
{
   return div_exact(a, long(b));
}
inline
Integer&& div_exact(Integer&& a, long b)
{
   return std::move(a.div_exact(b));
}
inline
Integer&& div_exact(Integer&& a, int b)
{
   return div_exact(std::move(a), long(b));
}

/// Logarithm (rounded down).
inline
int log2_floor(const Integer& a)
{
   if (__builtin_expect(!isfinite(a), 0)) throw GMP::NaN();
   int n=mpz_size(a.get_rep())-1;
   return n>=0 ? n*mp_bits_per_limb + log2_floor(mpz_getlimbn(a.get_rep(), n)) : 0;
}

/// Logarithm (rounded up). 
inline
int log2_ceil(const Integer& a)
{
   if (__builtin_expect(!isfinite(a),0)) throw GMP::NaN();
   int n=mpz_size(a.get_rep())-1;
   return n>=0 ? n*mp_bits_per_limb + log2_ceil(mpz_getlimbn(a.get_rep(), n)) : 0;
}

namespace operations {

template <>
struct divexact_scalar<Integer,Integer,Integer> {
   typedef Integer first_argument_type;
   typedef Integer second_argument_type;
   typedef const Integer result_type;

   result_type operator() (const Integer& a, const Integer& b) const { return div_exact(a,b); }
   void assign(Integer& a, const Integer& b) const { a.div_exact(b); }
};

template <>
struct divexact_scalar<Integer,long,Integer> {
   typedef Integer first_argument_type;
   typedef long second_argument_type;
   typedef const Integer result_type;

   result_type operator() (const Integer& a, long b) const { return div_exact(a,b); }
   void assign(Integer& a, long b) const { a.div_exact(b); }
};

template <>
struct divexact_scalar<Integer,int,Integer> : divexact_scalar<Integer,long,Integer> {
   typedef int second_argument_type;
};

template <typename T>
struct cmp_GMP_based : cmp_extremal, cmp_partial_scalar {

   typedef T first_argument_type;
   typedef T second_argument_type;
   typedef cmp_value result_type;

   using cmp_extremal::operator();
   using cmp_partial_scalar::operator();

   cmp_value operator() (const T& a, const T& b) const
   {
      return cmp_value(sign(a.compare(b)));
   }

   template <typename T2>
   typename std::enable_if<mlist_contains<typename T::is_comparable_with, T2>::value, cmp_value>::type
   operator() (const T& a, const T2 b) const
   {
      return cmp_value(sign(a.compare(b)));
   }

   template <typename T2>
   typename std::enable_if<!mlist_contains<typename T::is_comparable_with, T2>::value && mlist_contains<typename T2::is_comparable_with, T>::value, cmp_value>::type
   operator() (const T& a, const T2& b) const
   {
      return cmp_value(sign(-b.compare(a)));
   }

   template <typename T2>
   typename std::enable_if<mlist_contains<typename T::is_comparable_with, T2>::value, cmp_value>::type
   operator() (const T2 a, const T& b) const
   {
      return cmp_value(sign(-b.compare(a)));
   }

   template <typename T2>
   typename std::enable_if<!mlist_contains<typename T::is_comparable_with, T2>::value && mlist_contains<typename T2::is_comparable_with, T>::value, cmp_value>::type
   operator() (const T2& a, const T& b) const
   {
      return cmp_value(sign(a.compare(b)));
   }
};

template <>
struct cmp_scalar<Integer, Integer, void>
   : cmp_GMP_based<Integer> {};

} // end namespace operations

template <>
struct spec_object_traits<Integer>
   : spec_object_traits<is_scalar> {
   static
   bool is_zero(const Integer& a)
   {
      return a.is_zero();
   }

   static
   bool is_one(const Integer& a)
   {
      return a==1;
   }

   static const Integer& zero();
   static const Integer& one();
};

// this is needed for various adapters transferring GMP values from third-parties
template <>
struct isomorphic_types<mpz_t, Integer>
   : std::true_type {};

template <> struct hash_func<MP_INT, is_opaque> {
protected:
   size_t impl(mpz_srcptr a) const
   {
      size_t result=0;
      for (int i=0, n=mpz_size(a); i<n; ++i)
         (result <<= 1) ^= mpz_getlimbn(a, i);
      return result;
   }
public:
   size_t operator() (const MP_INT& a) const { return impl(&a); }
};

template <>
struct hash_func<Integer, is_scalar> : hash_func<MP_INT>
{
   size_t operator() (const Integer& a) const
   {
      return __builtin_expect(isfinite(a), 1) ? impl(a.get_rep()) : 0;
   }
};

template <>
Integer
pow(const Integer& base, long exp);

}

namespace polymake {
   using pm::Integer;
}

#endif // POLYMAKE_INTEGER_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
