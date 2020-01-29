/* Copyright (c) 1997-2020
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

/** @file Rational.h
    @brief Implementation of pm::Rational class
*/

#ifndef POLYMAKE_RATIONAL_H
#define POLYMAKE_RATIONAL_H

#include "polymake/Integer.h"

namespace pm {

/// data from third parties can't have infinite values
constexpr bool isfinite(const __mpq_struct&) { return true; }
constexpr Int isinf(const __mpq_struct&) { return 0; }

bool isfinite(const Rational& a) noexcept;
Int isinf(const Rational& a) noexcept;
Int sign(const Rational& a) noexcept;

template <> struct spec_object_traits<Rational>;

template <bool is_numerator, typename TPart=Integer>
class RationalParticle {
   friend class Rational;
   template <bool, typename> friend class RationalParticle;

   // TODO: make this class move-only when perl::Value is prepared for this
   // RationalParticle(const RationalParticle&) = delete;

   Rational* master;

   TPart& particle() noexcept;
   const TPart& particle() const noexcept;
   void canonicalize();

public:
   explicit RationalParticle(Rational& r) noexcept
      : master(&r) {}

   // RationalParticle(RationalParticle&& other) = default;

   template <typename T, typename=typename std::enable_if<can_assign_to<T, TPart>::value>::type>
   RationalParticle& operator= (T&& b)
   {
      particle()=std::forward<T>(b);
      canonicalize();
      return *this;
   }

   template <bool is_numerator2>
   RationalParticle& operator= (const RationalParticle<is_numerator2, TPart>& b)
   {
      if (is_numerator != is_numerator2 || master != b.master) {
         particle()=b.particle();
         canonicalize();
      }
      return *this;
   }

   void swap(TPart& b)
   {
      particle().swap(b);
      canonicalize();
   }

   template <bool is_numerator2>
   void swap(RationalParticle<is_numerator2, TPart>&& b)
   {
      if (is_numerator != is_numerator2 || master != b.master) {
         particle().swap(b.particle());
         canonicalize();
         b.canonicalize();
      }
   }

   operator const TPart& () const noexcept
   {
      return particle();
   }

   friend
   std::istream& operator>> (std::istream& in, RationalParticle& me)
   {
      in >> me.particle();
      me.canonicalize();
      return in;
   }

   RationalParticle& operator++()
   {
      ++particle();
      canonicalize();
      return *this;
   }

   RationalParticle& operator--()
   {
      --particle();
      canonicalize();
      return *this;
   }

   TPart operator++ (int)
   {
      TPart copy(particle());  operator++();  return copy;
   }

   TPart operator-- (int)
   {
      TPart copy(particle());  operator--();  return copy;
   }

   template <typename T>
   RationalParticle& operator+= (const T& b)
   {
      particle() += b;
      canonicalize();
      return *this;
   }

   template <typename T>
   RationalParticle& operator-= (const T& b)
   {
      particle() -= b;
      canonicalize();
      return *this;
   }

   template <typename T>
   RationalParticle& operator*= (const T& b)
   {
      particle() *= b;
      canonicalize();
      return *this;
   }

   template <typename T>
   RationalParticle& operator/= (const T& b)
   {
      particle() /= b;
      canonicalize();
      return *this;
   }

   template <typename T>
   RationalParticle& operator%= (const T& b)
   {
      particle() %= b;
      canonicalize();
      return *this;
   }

   template <typename T>
   RationalParticle& div_exact(const T& b)
   {
      particle().div_exact(b);
      canonicalize();
      return *this;
   }

   template <typename T>
   RationalParticle& operator<<= (const T& b)
   {
      particle() <<= b;
      canonicalize();
      return *this;
   }

   template <typename T>
   RationalParticle& operator>>= (const T& b)
   {
      particle() >>= b;
      canonicalize();
      return *this;
   }
};

/** @brief Rational number with unlimited precision.
 *
 *  Powered by GMP
 */
class Rational
   : protected __mpq_struct {

   typedef Integer::initialized initialized;
public:
   ~Rational() noexcept
   {
      if (mpq_denref(this)->_mp_d) mpq_clear(this);
   }

   /// Constructors

   Rational(const Rational& b)
   {
      set_data(b, initialized::no);
   }

   Rational(Rational&& b) noexcept
   {
      set_data(b, initialized::no);
   }

   /// Copy the value from a third party
   explicit Rational(const mpq_t& b)
   {
      set_data(b[0], initialized::no);
   }

   /// Steal the value from a third party
   /// The source must be re-initialized if it's going to be used afterwards
   explicit Rational(mpq_t&& b) noexcept
   {
      set_data(b[0], initialized::no);
   }

   /// Create a Rational from an integral number

   Rational(const Integer& b)
      : Rational(b, 1) {}

   Rational(Integer&& b)
      : Rational(std::move(b), 1) {}

   /// Copy the numerator value from a third party
   explicit Rational(const mpz_t& b)
   {
      set_data(b[0], 1, initialized::no);
   }

   /// Steal the numerator value from a third party
   /// The source must be re-initialized if it's going to be used afterwards
   explicit Rational(mpz_t&& b)
   {
      set_data(b[0], 1, initialized::no);
   }

   Rational(long b = 0)
      : Rational(b, 1) {}

   Rational(int b)
      : Rational(long(b), 1) {}

   /// Construct an infinite value
   static
   Rational infinity(Int sgn)
   {
      assert(sgn == 1 || sgn == -1);
      Rational result(nullptr);
      set_inf(&result, sgn, initialized::no);
      return result;
   }

   /// Create a Rational as the quotient of two integrals, represented as Integer, long, or int.

   Rational(const Integer& num, const Integer& den)
   {
      set_data(num, den, initialized::no);
   }
   Rational(Integer&& num, Integer&& den)
   {
      set_data(num, den, initialized::no);
   }
   Rational(const Integer& num, Integer&& den)
   {
      set_data(num, den, initialized::no);
   }
   Rational(Integer&& num, const Integer& den)
   {
      set_data(num, den, initialized::no);
   }
   Rational(const Integer& num, long den)
   {
      set_data(num, den, initialized::no);
   }
   Rational(Integer&& num, long den)
   {
      set_data(num, den, initialized::no);
   }
   Rational(long num, const Integer& den)
   {
      set_data(num, den, initialized::no);
   }
   Rational(long num, Integer&& den)
   {
      set_data(num, den, initialized::no);
   }
   Rational(long num, long den)
   {
      set_data(num, den, initialized::no);
   }

   /// Copy or steal numerator and denominator values from a third party

   Rational(const mpz_t& num, const mpz_t& den)
   {
      set_data(num[0], den[0], initialized::no);
   }
   Rational(mpz_t&& num, mpz_t&& den)
   {
      set_data(num[0], den[0], initialized::no);
   }
   Rational(const mpz_t& num, mpz_t&& den)
   {
      set_data(num[0], den[0], initialized::no);
   }
   Rational(mpz_t&& num, const mpz_t& den)
   {
      set_data(num[0], den[0], initialized::no);
   }

   Rational(double b)
   {
      if (__builtin_expect(isfinite(b), 1)) {
         mpq_init(this);
         mpq_set_d(this, b);
      } else {
         set_inf(this, isinf(b), initialized::no);
      }
   }

   explicit Rational(const AccurateFloat& b)
   {
      mpq_init(this);
      *this=b;
   }

   /// Parse a string "num/den", "num", or "±inf"
   explicit Rational(const char* s);

   Rational(gmp_randstate_t rnd, unsigned long bits)
   {
      mpq_init(this);
      mpz_urandomb(mpq_numref(this), rnd, bits);
      mpq_div_2exp(this, this, bits);
   }

#if defined(__GMP_PLUSPLUS__)
   //constructs from gmp's mpz_class as numerator and denominator
   Rational(const mpz_class& num, const mpz_class& den)
   {
      mpz_init_set(mpq_numref(this), num.get_mpz_t());
      mpz_init_set(mpq_denref(this), den.get_mpz_t());
      canonicalize();
   }
#endif

   /// Assignment

   Rational& operator= (const Rational& b)
   {
      set_data(b, initialized::yes);
      return *this;
   }

   Rational& operator= (Rational&& b) noexcept
   {
      set_data(b, initialized::yes);
      return *this;
   }

   Rational& operator= (const Integer& b)
   {
      set_data(b, 1, initialized::yes);
      return *this;
   }

   Rational& operator= (Integer&& b)
   {
      set_data(b, 1, initialized::yes);
      return *this;
   }

   Rational& operator= (long b)
   {
      set_data(b, 1, initialized::yes);
      return *this;
   }

   Rational& operator= (int b)
   {
      return operator=(long(b));
   }

   Rational& operator= (double b)
   {
      if (__builtin_expect(isfinite(b), 1)) {
         if (__builtin_expect(!isfinite(*this), 0))
            mpq_init(this);
         mpq_set_d(this, b);
      } else {
         set_inf(this, isinf(b));
      }
      return *this;
   }

   Rational& operator= (const AccurateFloat&);

   /// Assign a copy of data obtained from a third party
   Rational& copy_from(mpq_srcptr src)
   {
      set_data(src[0], initialized::yes);
      return *this;
   }

   Rational& copy_from(mpz_srcptr num_src)
   {
      set_data(num_src[0], 1, initialized::yes);
      return *this;
   }

   /// Assign numerator and denominator from separate sources

   Rational& set(const Integer& num, const Integer& den)
   {
      set_data(num, den, initialized::yes);
      return *this;
   }
   Rational& set(Integer&& num, Integer&& den)
   {
      set_data(num, den, initialized::yes);
      return *this;
   }
   Rational& set(const Integer& num, Integer&& den)
   {
      set_data(num, den, initialized::yes);
      return *this;
   }
   Rational& set(Integer&& num, const Integer& den)
   {
      set_data(num, den, initialized::yes);
      return *this;
   }
   Rational& set(const Integer& num, long den)
   {
      set_data(num, den, initialized::yes);
      return *this;
   }
   Rational& set(Integer&& num, long den)
   {
      set_data(num, den, initialized::yes);
      return *this;
   }
   Rational& set(long num, const Integer& den)
   {
      set_data(num, den, initialized::yes);
      return *this;
   }
   Rational& set(long num, Integer&& den)
   {
      set_data(num, den, initialized::yes);
      return *this;
   }
   Rational& set(long num, long den)
   {
      set_data(num, den, initialized::yes);
      return *this;
   }
   Rational& set(mpz_t&& num, mpz_t&& den)
   {
      set_data(num[0], den[0], initialized::yes);
      return *this;
   }

   Rational& copy_from(mpz_srcptr num_src, mpz_srcptr den_src)
   {
      set_data(num_src[0], den_src[0], initialized::yes);
      return *this;
   }

   Rational& set(const char* s)
   {
      if (__builtin_expect(!isfinite(*this), 0))
         mpz_init(mpq_numref(this));
      parse(s);
      return *this;
   }

   /// Exchange the values.
   void swap(Rational& b)
   {
      mpq_swap(this, &b);
   }

   // TODO: kill this
   friend void relocate(Rational* from, Rational* to)
   {
      static_cast<__mpq_struct&>(*to)=*from;
   }

   /// Separate access to numerator and denominator

   typedef RationalParticle<true> num_proxy;
   typedef RationalParticle<false> den_proxy;

   friend
   const Integer& numerator(const Rational& a) noexcept
   {
      return a.num_ref();
   }

   friend
   const Integer& numerator_if_integral(const Rational& a)
   {
      if (!a.is_integral())
         throw GMP::BadCast("non-integral number");
      return a.num_ref();
   }

   friend
   num_proxy numerator(Rational& a) noexcept
   {
      return num_proxy(a);
   }

   friend
   const Integer& denominator(const Rational& a) noexcept
   {
      return a.den_ref();
   }

   friend
   den_proxy denominator(Rational& a) noexcept
   {
      return den_proxy(a);
   }

   /// Cast to simpler types

   explicit operator double() const
   {
      if (__builtin_expect(isfinite(*this), 1))
         return mpq_get_d(this);
      else
         return double(isinf(*this)) * std::numeric_limits<double>::infinity();
   }

   explicit operator long() const
   {
      if (!is_integral())
         throw GMP::BadCast("non-integral number");
      return static_cast<long>(numerator(*this));
   }

   mpq_srcptr get_rep() const noexcept { return this; }

   /// Unary operators

   friend
   Rational operator+ (const Rational& b)
   {
      return b;
   }

   friend
   Rational&& operator+ (Rational&& b)
   {
      return std::move(b);
   }

   /// Increment

   Rational& operator++ ()
   {
      if (__builtin_expect(isfinite(*this), 1))
         mpz_add(mpq_numref(this), mpq_numref(this), mpq_denref(this));
      return *this;
   }

   Rational operator++ (int) { Rational copy(*this); operator++(); return copy; }

   Rational& operator-- ()
   {
      if (__builtin_expect(isfinite(*this), 1))
         mpz_sub(mpq_numref(this), mpq_numref(this), mpq_denref(this));
      return *this;
   }

   Rational operator-- (int) { Rational copy(*this); operator--(); return copy; }

   /// In-place negation.
   Rational& negate()
   {
      static_cast<Integer*>(mpq_numref(this))->negate();
      return *this;
   }

   /// Negation
   friend
   Rational operator- (const Rational& a)
   {
      Rational result(a);
      result.negate();
      return result;
   }

   friend
   Rational&& operator- (Rational&& a)
   {
      return std::move(a.negate());
   }


   /// Addition

   Rational& operator+= (const Rational& b)
   {
      if (__builtin_expect(isfinite(*this), 1)) {
         if (__builtin_expect(isfinite(b), 1))
            mpq_add(this, this, &b);
         else
            set_inf(this, 1, b);
      } else {
         if (isinf(*this)+isinf(b) == 0)
            throw GMP::NaN();
      }
      return *this;
   }

   friend
   Rational operator+ (const Rational& a, const Rational& b)
   {
      Rational result;
      if (__builtin_expect(isfinite(a), 1)) {
         if (__builtin_expect(isfinite(b), 1))
            mpq_add(&result, &a, &b);
         else
            set_inf(&result, 1, b);
      } else {
         if (isinf(a)+isinf(b) == 0)
            throw GMP::NaN();
         set_inf(&result, a);
      }
      return result;
   }

   friend
   Rational&& operator+ (Rational&& a, const Rational& b)
   {
      return std::move(a+=b);
   }
   friend
   Rational&& operator+ (const Rational& a, Rational&& b)
   {
      return std::move(b+=a);
   }
   friend
   Rational&& operator+ (Rational&& a, Rational&& b)
   {
      return std::move(a+=b);
   }

   Rational& operator+= (const Integer& b)
   {
      if (__builtin_expect(isfinite(*this), 1)) {
         if (__builtin_expect(isfinite(b), 1))
            mpz_addmul(mpq_numref(this), mpq_denref(this), &b);
         else
            set_inf(this, 1, b);
      } else {
         if (isinf(*this)+isinf(b) == 0)
            throw GMP::NaN();
      }
      return *this;
   }

   friend
   Rational operator+ (const Rational& a, const Integer& b)
   {
      Rational result;
      if (__builtin_expect(isfinite(a), 1)) {
         if (__builtin_expect(isfinite(b), 1)) {
            mpq_set(&result, &a);
            mpz_addmul(mpq_numref(&result), mpq_denref(&a), b.get_rep());
         } else {
            set_inf(&result, 1, b);
         }
      } else {
         if (isinf(a)+isinf(b) == 0)
            throw GMP::NaN();
         set_inf(&result, a);
      }
      return result;
   }

   friend
   Rational operator+ (const Integer& a, const Rational& b)
   {
      return b+a;
   }
   friend
   Rational&& operator+ (Rational&& a, const Integer& b)
   {
      return std::move(a+=b);
   }
   friend
   Rational&& operator+ (const Integer& a, Rational&& b)
   {
      return std::move(b+=a);
   }

   Rational& operator+= (long b)
   {
      if (__builtin_expect(isfinite(*this), 1)) {
         if (b>=0)
            mpz_addmul_ui(mpq_numref(this), mpq_denref(this), b);
         else
            mpz_submul_ui(mpq_numref(this), mpq_denref(this), -b);
      }
      return *this;
   }
   Rational& operator+= (int b)
   {
      return *this += long(b);
   }

   friend
   Rational operator+ (const Rational& a, long b)
   {
      Rational result(a);
      result+=b;
      return result;
   }
   friend
   Rational&& operator+ (Rational&& a, long b)
   {
      return std::move(a+=b);
   }
   friend
   Rational operator+ (const Rational& a, int b)
   {
      return a+long(b);
   }
   friend
   Rational&& operator+ (Rational&& a, int b)
   {
      return std::move(a+=long(b));
   }
   friend
   Rational operator+ (long a, const Rational& b)
   {
      return b+a;
   }
   friend
   Rational operator+ (int a, const Rational& b)
   {
      return b+long(a);
   }
   friend
   Rational&& operator+ (long a, Rational&& b)
   {
      return std::move(b+=a);
   }
   friend
   Rational&& operator+ (int a, Rational&& b)
   {
      return std::move(b+=long(a));
   }


   /// Subtraction

   Rational& operator-= (const Rational& b)
   {
      if (__builtin_expect(isfinite(*this), 1)) {
         if (__builtin_expect(isfinite(b), 1))
            mpq_sub(this, this, &b);
         else
            set_inf(this, -1, b);
      } else {
         if (isinf(*this) == isinf(b))
            throw GMP::NaN();
      }
      return *this;
   }

   friend
   Rational operator- (const Rational& a, const Rational& b)
   {
      Rational result;
      if (__builtin_expect(isfinite(a), 1)) {
         if (__builtin_expect(isfinite(b), 1))
            mpq_sub(&result, &a, &b);
         else
            set_inf(&result, -1, b);
      } else {
         if (isinf(a) == isinf(b))
            throw GMP::NaN();
         set_inf(&result, a);
      }
      return result;
   }

   friend
   Rational&& operator- (Rational&& a, const Rational& b)
   {
      return std::move(a-=b);
   }
   friend
   Rational&& operator- (const Rational& a, Rational&& b)
   {
      return std::move((b-=a).negate());
   }
   friend
   Rational&& operator- (Rational&& a, Rational&& b)
   {
      return std::move(a-=b);
   }

   Rational& operator-= (const Integer& b)
   {
      if (__builtin_expect(isfinite(*this), 1)) {
         if (__builtin_expect(isfinite(b), 1))
            mpz_submul(mpq_numref(this), mpq_denref(this), &b);
         else
            set_inf(this, -1, b);
      } else {
         if (isinf(*this) == isinf(b))
            throw GMP::NaN();
      }
      return *this;
   }

   friend
   Rational operator- (const Rational& a, const Integer& b)
   {
      Rational result;
      if (__builtin_expect(isfinite(a), 1)) {
         if (__builtin_expect(isfinite(b), 1)) {
            mpq_set(&result, &a);
            mpz_submul(mpq_numref(&result), mpq_denref(&a), b.get_rep());
         } else {
            set_inf(&result, -1, b);
         }
      } else {
         if (isinf(a) == isinf(b))
            throw GMP::NaN();
         set_inf(&result, a);
      }
      return result;
   }

   friend
   Rational&& operator- (Rational&& a, const Integer& b)
   {
      return std::move(a-=b);
   }
   friend
   Rational operator- (const Integer& a, const Rational& b)
   {
      Rational result(b-a);
      result.negate();
      return result;
   }
   friend
   Rational&& operator- (const Integer& a, Rational&& b)
   {
      return std::move((b-=a).negate());
   }

   Rational& operator-= (long b)
   {
      if (__builtin_expect(isfinite(*this), 1)) {
         if (b>=0)
            mpz_submul_ui(mpq_numref(this), mpq_denref(this), b);
         else
            mpz_addmul_ui(mpq_numref(this), mpq_denref(this), -b);
      }
      return *this;
   }
   Rational& operator-= (int b)
   {
      return *this -= long(b);
   }

   friend
   Rational operator- (const Rational& a, long b)
   {
      Rational result(a);
      result-=b;
      return result;
   }
   friend
   Rational&& operator- (Rational&& a, long b)
   {
      return std::move(a-=b);
   }
   friend
   Rational operator- (long a, const Rational& b)
   {
      Rational result(b-a);
      result.negate();
      return result;
   }
   friend
   Rational&& operator- (long a, Rational&& b)
   {
      return std::move((b-=a).negate());
   }
   friend
   Rational operator- (const Rational& a, int b)
   {
      return a-long(b);
   }
   friend
   Rational&& operator- (Rational&& a, int b)
   {
      return std::move(a-=long(b));
   }
   friend
   Rational operator- (int a, const Rational& b)
   {
      return long(a)-b;
   }
   friend
   Rational&& operator- (int a, Rational&& b)
   {
      return long(a)-std::move(b);
   }


   /// Multiplication

   Rational& operator*= (const Rational& b)
   {
      if (__builtin_expect(isfinite(*this), 1)) {
         if (__builtin_expect(isfinite(b), 1))
            mpq_mul(this, this, &b);
         else
            set_inf(this, mpq_sgn(this), b);
      } else {
         inf_inv_sign(this, mpq_sgn(&b));
      }
      return *this;
   }

   friend
   Rational operator* (const Rational& a, const Rational& b)
   {
      Rational result;
      if (__builtin_expect(isfinite(a), 1)) {
         if (__builtin_expect(isfinite(b), 1))
            mpq_mul(&result, &a, &b);
         else
            set_inf(&result, mpq_sgn(&a), b);
      } else {
         set_inf(&result, mpq_sgn(&b), a);
      }
      return result;
   }

   friend
   Rational&& operator* (Rational&& a, const Rational& b)
   {
      return std::move(a*=b);
   }
   friend
   Rational&& operator* (const Rational& a, Rational&& b)
   {
      return std::move(b*=a);
   }
   friend
   Rational&& operator* (Rational&& a, Rational&& b)
   {
      return std::move(a*=b);
   }

   Rational& operator*= (const Integer& b)
   {
      if (__builtin_expect(isfinite(*this), 1)) {
         if (__builtin_expect(isfinite(b), 1)) {
            mult_with_Integer(*this, b);
         } else {
            set_inf(this, mpq_sgn(this), b);
         }
      } else {
         inf_inv_sign(this, mpz_sgn(&b));
      }
      return *this;
   }

   friend
   Rational operator* (const Rational& a, const Integer& b)
   {
      Rational result;
      if (__builtin_expect(isfinite(a), 1)) {
         if (__builtin_expect(isfinite(b), 1)) {
            result.mult_with_Integer(a, b);
         } else {
            set_inf(&result, mpq_sgn(&a), b);
         }
      } else {
         set_inf(&result, mpz_sgn(b.get_rep()), a);
      }
      return result;
   }

   friend
   Rational&& operator* (Rational&& a, const Integer& b)
   {
      return std::move(a*=b);
   }
   friend
   Rational operator* (const Integer& a, const Rational& b)
   {
      return b*a;
   }
   friend
   Rational&& operator* (const Integer& a, Rational&& b)
   {
      return std::move(b*=a);
   }

   Rational& operator*= (long b);
   Rational& operator*= (int b)
   {
      return *this *= long(b);
   }

   friend
   Rational operator* (const Rational& a, long b)
   {
      Rational result(a);
      result *= b;
      return result;
   }
   friend
   Rational&& operator* (Rational&& a, long b)
   {
      return std::move(a*=b);
   }
   friend
   Rational operator* (const Rational& a, int b)
   {
      return a*long(b);
   }
   friend
   Rational&& operator* (Rational&& a, int b)
   {
      return std::move(a*=long(b));
   }
   friend
   Rational operator* (long a, const Rational& b)
   {
      return b*a;
   }
   friend
   Rational operator* (long a, Rational&& b)
   {
      return std::move(b*=a);
   }
   friend
   Rational operator* (int a, const Rational& b)
   {
      return b*long(a);
   }
   friend
   Rational&& operator* (int a, Rational&& b)
   {
      return std::move(b*=long(a));
   }

   /// Inversion (reciprocal value)

   friend
   Rational inv(const Rational& a)
   {
      Rational result;
      if (__builtin_expect(isfinite(a), 1)) {
         if (__builtin_expect(!a.is_zero(), 1))
            mpq_inv(&result, &a);
         else
            set_inf(&result, 1);
      }
      return result;
   }

   friend
   Rational&& inv(Rational&& a)
   {
      if (__builtin_expect(isfinite(a), 1)) {
         if (__builtin_expect(!a.is_zero(), 1)) {
            mpz_swap(mpq_numref(&a), mpq_denref(&a));
            a.canonicalize_sign();
         } else {
            set_inf(&a, 1);
         }
      } else {
         a=0;
      }
      return std::move(a);
   }

   /// Division

   Rational& operator/= (const Rational& b)
   {
      if (__builtin_expect(isfinite(*this), 1)) {
         if (__builtin_expect(b.is_zero(), 0))
            throw GMP::ZeroDivide();
         if (__builtin_expect(!is_zero(), 1)) {
            if (__builtin_expect(isfinite(b), 1))
               mpq_div(this, this, &b);
            else
               *this=0;
         }
      } else {
         if (!isfinite(b))
            throw GMP::NaN();
         inf_inv_sign(this, mpq_sgn(&b));
      }
      return *this;
   }

   friend
   Rational operator/ (const Rational& a, const Rational& b)
   {
      Rational result;
      if (__builtin_expect(isfinite(a), 1)) {
         if (__builtin_expect(b.is_zero(), 0))
            throw GMP::ZeroDivide();
         if (__builtin_expect(!a.is_zero(), 1) &&
             __builtin_expect(isfinite(b), 1))
            mpq_div(&result, &a, &b);
      } else {
         if (!isfinite(b))
            throw GMP::NaN();
         set_inf(&result, mpq_sgn(&a), b);
      }
      return result;
   }

   friend
   Rational&& operator/ (Rational&& a, const Rational& b)
   {
      return std::move(a/=b);
   }

   friend
   Rational&& operator/ (const Rational& a, Rational&& b)
   {
      if (__builtin_expect(isfinite(a), 1)) {
         if (__builtin_expect(b.is_zero(), 0))
            throw GMP::ZeroDivide();
         if (__builtin_expect(!a.is_zero(), 1) &&
             __builtin_expect(isfinite(b), 1))
            mpq_div(&b, &a, &b);
         else
            b=0;
      } else {
         if (!isfinite(b))
            throw GMP::NaN();
         inf_inv_sign(&b, mpq_sgn(&a));
      }
      return std::move(b);
   }

   friend
   Rational&& operator/ (Rational&& a, Rational&& b)
   {
      return std::move(a/=b);
   }

   Rational& operator/= (const Integer& b)
   {
      if (__builtin_expect(isfinite(*this), 1)) {
         if (__builtin_expect(isfinite(b), 1))
            div_thru_Integer(*this, b);
         else
            *this=0;
      } else {
         if (!isfinite(b))
            throw GMP::NaN();
         inf_inv_sign(this, mpz_sgn(&b));
      }
      return *this;
   }

   friend
   Rational operator/ (const Rational& a, const Integer& b)
   {
      Rational result;
      if (__builtin_expect(isfinite(a), 1)) {
         if (__builtin_expect(isfinite(b), 1))
            result.div_thru_Integer(a, b);
         else
            result=0;
      } else {
         if (!isfinite(b))
            throw GMP::NaN();
         set_inf(&result, mpq_sgn(&a), b);
      }
      return result;
   }

   friend
   Rational&& operator/ (Rational&& a, const Integer& b)
   {
      return std::move(a/=b);
   }

   friend
   Rational operator/ (const Integer& a, const Rational& b)
   {
      if (b.is_zero())
         throw GMP::ZeroDivide();
      return inv(b)*a;
   }

   friend
   Rational&& operator/ (const Integer& a, Rational&& b)
   {
      if (b.is_zero())
         throw GMP::ZeroDivide();
      return inv(std::move(b))*a;
   }

   Rational& operator/= (long b);

   Rational& operator/= (int b)
   {
      return *this /= long(b);
   }
   friend
   Rational operator/ (const Rational& a, long b)
   {
      Rational result(a);
      result /= b;
      return result;
   }
   friend
   Rational&& operator/ (Rational&& a, long b)
   {
      return std::move(a/=b);
   }
   friend
   Rational operator/ (const Rational& a, int b)
   {
      return a/long(b);
   }
   friend
   Rational&& operator/ (Rational&& a, int b)
   {
      return std::move(a)/long(b);
   }
   friend
   Rational operator/ (long a, const Rational& b)
   {
      if (b.is_zero())
         throw GMP::ZeroDivide();
      return inv(b)*a;
   }
   friend
   Rational&& operator/ (long a, Rational&& b)
   {
      if (b.is_zero())
         throw GMP::ZeroDivide();
      return inv(std::move(b))*a;
   }
   friend
   Rational operator/ (int a, const Rational& b)
   {
      return long(a)/b;
   }
   friend
   Rational&& operator/ (int a, Rational&& b)
   {
      return long(a)/std::move(b);
   }


   /// Multiply with or divide through 2**k.

   Rational& operator<<= (long k)
   {
      if (__builtin_expect(isfinite(*this), 1)) {
         if (k >= 0)
            mpq_mul_2exp(this, this, k);
         else
            mpq_div_2exp(this, this, -k);
      }
      return *this;
   }
   Rational& operator<<= (int k)
   {
      return *this <<= long(k);
   }

   friend
   Rational operator<< (const Rational& a, long k)
   {
      Rational result(a);
      result <<= k;
      return result;
   }
   friend
   Rational&& operator<< (Rational&& a, long k)
   {
      return std::move(a <<= k);
   }
   friend
   Rational operator<< (const Rational& a, int k)
   {
      return a << long(k);
   }
   friend
   Rational&& operator<< (Rational&& a, int k)
   {
      return std::move(a <<= long(k));
   }

   /// Divide through or multiply with 2**k.

   Rational& operator>>= (long k)
   {
      return *this <<= -k;
   }
   Rational& operator>>= (int k)
   {
      return *this >>= long(k);
   }

   friend
   Rational operator>> (const Rational& a, long k)
   {
      Rational result(a);
      result >>= k;
      return result;
   }
   friend
   Rational&& operator>> (Rational&& a, long k)
   {
      return std::move(a >>= k);
   }
   friend
   Rational operator>> (const Rational& a, int k)
   {
      return a >> long(k);
   }
   friend
   Rational&& operator>> (Rational&& a, int k)
   {
      return std::move(a >>= long(k));
   }


   /// The closest integral not greater than this
   Integer floor() const
   {
      if (__builtin_expect(isfinite(*this), 1)) {
         Integer result;
         mpz_fdiv_q(&result, mpq_numref(this), mpq_denref(this));
         return result;
      }
      return Integer::infinity(mpq_sgn(this));
   }

   /// The closest integral not smaller than the given value
   Integer ceil() const
   {
      if (__builtin_expect(isfinite(*this), 1)) {
         Integer result;
         mpz_cdiv_q(&result, mpq_numref(this), mpq_denref(this));
         return result;
      }
      return Integer::infinity(mpq_sgn(this));
   }

   /// Truncation toward zero
   Integer trunc() const
   {
      if (__builtin_expect(isfinite(*this), 1)) {
         Integer result;
         mpz_tdiv_q(&result, mpq_numref(this), mpq_denref(this));
         return result;
      }
      return Integer::infinity(mpq_sgn(this));
   }

   /// fast comparison with 0
   bool is_zero() const noexcept
   {
      return mpq_sgn(this)==0;
   }

   /// check whether the denominator equals 1
   /// for infinite values, returns true as well
   bool is_integral() const noexcept
   {
      return !mpz_cmp_ui(mpq_denref(this), 1);
   }

   /// Comparison. 
   /// The magnitude of the return value is arbitrary, only its sign is relevant.

   Int compare(const Rational& b) const
   {
      if (__builtin_expect(isfinite(*this) && isfinite(b), 1))
         return mpq_cmp(this, &b);
      else
         return isinf(*this)-isinf(b);
   }

   Int compare(const Integer& b) const
   {
      if (__builtin_expect(isfinite(*this) && isfinite(b), 1)) {
         if (__builtin_expect(b.is_zero(), 0))
            return mpq_sgn(this);
         return is_integral() 
                ? numerator(*this).compare(b)
                : numerator(*this).compare(b*denominator(*this));
      } else {
         return isinf(*this)-isinf(b);
      }
   }

   Int compare(long b) const
   {
      if (__builtin_expect(isfinite(*this), 1)) {
         if (__builtin_expect(!b, 0))
            return mpq_sgn(this);
         return is_integral()
                ? numerator(*this).compare(b)
                : numerator(*this).compare(b*denominator(*this));
      } else {
         return isinf(*this);
      }
   }

   Int compare(int b) const { return compare(long(b)); }

   /// Comparison. 
   Int compare(double b) const
   {
      if (__builtin_expect(isfinite(*this) && isfinite(b), 1)) {
         return is_integral()
                ? mpz_cmp_d(mpq_numref(this), b)
                : sign(double(*this)-b);
      } else {
         return isinf(*this)-isinf(b);
      }
   }

   typedef mlist<int, long, double, Integer> is_comparable_with;
   typedef mlist_concat<Rational, is_comparable_with>::type self_and_comparable_with;

   friend
   bool operator== (const Rational& a, const Rational& b)
   {
      if (__builtin_expect(isfinite(a) && isfinite(b), 1))
         return mpq_equal(&a, &b);
      return isinf(a) == isinf(b);
   }

   friend
   bool operator== (const Rational& a, const Integer& b)
   {
      if (__builtin_expect(isfinite(a) && isfinite(b), 1))
         return a.is_integral() && numerator(a)==b;
      return isinf(a) == isinf(b);
   }

   friend
   bool operator== (const Rational& a, double b)
   {
      return a.compare(b)==0;
   }

   friend
   bool operator== (const Rational& a, long b)
   {
      return isfinite(a)
          && a.is_integral()
          && numerator(a)==b;
   }

   friend
   bool operator== (const Rational& a, int b)
   {
      return a==long(b);
   }

   template <typename T, typename=typename std::enable_if<mlist_contains<self_and_comparable_with, T>::value>::type>
   friend
   bool operator!= (const Rational& a, const T& b)
   {
      return !(a==b);
   }
   template <typename T, typename=typename std::enable_if<mlist_contains<self_and_comparable_with, T>::value>::type>
   friend
   bool operator< (const Rational& a, const T& b)
   {
      return a.compare(b)<0;
   }
   template <typename T, typename=typename std::enable_if<mlist_contains<self_and_comparable_with, T>::value>::type>
   friend
   bool operator> (const Rational& a, const T& b)
   {
      return a.compare(b)>0;
   }
   template <typename T, typename=typename std::enable_if<mlist_contains<self_and_comparable_with, T>::value>::type>
   friend
   bool operator<= (const Rational& a, const T& b)
   {
      return a.compare(b)<=0;
   }
   template <typename T, typename=typename std::enable_if<mlist_contains<self_and_comparable_with, T>::value>::type>
   friend
   bool operator>= (const Rational& a, const T& b)
   {
      return a.compare(b)>=0;
   }
   template <typename T, typename=typename std::enable_if<mlist_contains<is_comparable_with, T>::value>::type>
   friend
   bool operator== (const T& a, const Rational& b)
   {
      return b==a;
   }
   template <typename T, typename=typename std::enable_if<mlist_contains<is_comparable_with, T>::value>::type>
   friend
   bool operator!= (const T& a, const Rational& b)
   {
      return !(b==a);
   }
   template <typename T, typename=typename std::enable_if<mlist_contains<is_comparable_with, T>::value>::type>
   friend
   bool operator< (const T& a, const Rational& b)
   {
      return b>a;
   }
   template <typename T, typename=typename std::enable_if<mlist_contains<is_comparable_with, T>::value>::type>
   friend
   bool operator> (const T& a, const Rational& b)
   {
      return b<a;
   }
   template <typename T, typename=typename std::enable_if<mlist_contains<is_comparable_with, T>::value>::type>
   friend
   bool operator<= (const T& a, const Rational& b)
   {
      return b>=a;
   }
   template <typename T, typename=typename std::enable_if<mlist_contains<is_comparable_with, T>::value>::type>
   friend
   bool operator>= (const T& a, const Rational& b)
   {
      return b<=a;
   }

   friend
   bool abs_equal(const Rational& a, const Rational& b)
   {
      const bool f1=isfinite(a), f2=isfinite(b);
      if (__builtin_expect(f1 && f2, 1))
         return !mpz_cmp(mpq_denref(&a), mpq_denref(&b)) &&
                !mpz_cmpabs(mpq_numref(&a), mpq_numref(&b));
      return f1==f2;
   }
   friend
   bool abs_equal(const Rational& a, const Integer& b)
   {
      const bool f1=isfinite(a), f2=isfinite(b);
      if (__builtin_expect(f1 && f2, 1))
         return a.is_integral() &&
                !mpz_cmpabs(mpq_numref(&a), b.get_rep());
      return f1==f2;
   }
   friend
   bool abs_equal(const Rational& a, long b)
   {
      return isfinite(a) && a.is_integral() &&
             mpz_fits_slong_p(mpq_numref(&a)) && std::abs(mpz_get_si(mpq_numref(&a)))==std::abs(b);
   }
   friend
   bool abs_equal(const Rational& a, int b)
   {
      return abs_equal(a, long(b));
   }
   friend
   bool abs_equal(const Rational& a, double b)
   {
      const double da(a);
      return da==b || da==-b;
   }

   template <typename T, typename=typename std::enable_if<mlist_contains<is_comparable_with, T>::value>::type>
   friend
   bool abs_equal(const T& a, const Rational& b)
   {
      return abs_equal(b, a);
   }

   friend
   Rational abs(const Rational& a)
   {
      Rational result;
      if (__builtin_expect(isfinite(a), 1)) {
         mpz_abs(mpq_numref(&result), mpq_numref(&a));
         mpz_set(mpq_denref(&result), mpq_denref(&a));
      } else {
         set_inf(&result, 1);
      }
      return result;
   }

   friend
   Rational&& abs(Rational&& a)
   {
      mpz_abs(mpq_numref(&a), mpq_numref(&a));
      return std::move(a);
   }

   /// Power.

   static
   Rational pow(const Rational& a, long k)
   {
      Rational result;
      if (__builtin_expect(isfinite(a), 1)) {
         if (k >= 0) {
            mpz_pow_ui(mpq_numref(&result), mpq_numref(&a), k);
            mpz_pow_ui(mpq_denref(&result), mpq_denref(&a), k);
         } else {
            if (a.is_zero())
               throw GMP::ZeroDivide();
            mpz_pow_ui(mpq_numref(&result), mpq_denref(&a), -k);
            mpz_pow_ui(mpq_denref(&result), mpq_numref(&a), -k);
            result.canonicalize_sign();
         }
      } else {
         if (!k)
            throw GMP::NaN();
         set_inf(&result, k%2 ? mpq_sgn(&a) : 1);
      }
      return result;
   }

   static
   Rational pow(long a, long k)
   {
      Rational result;
      if (k >= 0) {
         mpz_ui_pow_ui(mpq_numref(&result), std::abs(a), k);
      } else if (__builtin_expect(a, 1)) {
         mpz_set_ui(mpq_numref(&result), 1);
         mpz_ui_pow_ui(mpq_denref(&result), std::abs(a), -k);
      } else {
         throw GMP::ZeroDivide();
      }
      if (a<0 && k%2)
         result.negate();
      return result;
   }

   static
   Rational pow(int a, long k)
   {
      return pow(long(a), k);
   }

   /// Power
   static
   Rational pow(const Integer& a, long k)
   {
      Rational result;
      if (__builtin_expect(isfinite(a), 1)) {
         if (k >= 0) {
            mpz_pow_ui(mpq_numref(&result), &a, k);
         } else if (__builtin_expect(!a.is_zero(), 1)) {
            mpz_set_ui(mpq_numref(&result), 1);
            mpz_pow_ui(mpq_denref(&result), &a, -k);
            result.canonicalize_sign();
         } else {
            throw GMP::ZeroDivide();
         }
      } else {
         if (!k)
            throw GMP::NaN();
         set_inf(&result, k%2 ? mpz_sgn(a.get_rep()) : 1);
      }
      return result;
   }

   void read(std::istream& is);
   void write(std::ostream& os) const;

   friend
   std::istream& operator>> (std::istream& is, Rational& a)
   {
      a.read(is);
      return is;
   }

   friend
   std::ostream& operator<< (std::ostream& os, const Rational& a)
   {
      a.write(os);
      return os;
   }

#if POLYMAKE_DEBUG
   void dump() const __attribute__((used)) { std::cerr << *this << std::flush; }
#endif

protected:
   /// uninitialized object
   explicit Rational(std::nullptr_t) {}

   void mult_with_Integer(const Rational& a, const Integer& b);
   void div_thru_Integer(const Rational& a, const Integer& b);

   static
   void set_inf(mpq_ptr me, Int sign, initialized st=initialized::yes)
   {
      Integer::set_inf(mpq_numref(me), sign, st);
      Integer::set_finite(mpq_denref(me), 1, st);
   }
   static
   void set_inf(mpq_ptr me, const Integer& from, initialized st=initialized::yes)
   {
      Integer::set_inf(mpq_numref(me), from, st);
      Integer::set_finite(mpq_denref(me), 1, st);
   }
   static
   void set_inf(mpq_ptr me, const Rational& from, initialized st=initialized::yes)
   {
      Integer::set_inf(mpq_numref(me), from.num_ref(), st);
      Integer::set_finite(mpq_denref(me), 1, st);
   }
   static
   void set_inf(mpq_ptr me, Int sign, long inv, initialized st=initialized::yes)
   {
      Integer::set_inf(mpq_numref(me), sign, inv, st);
      Integer::set_finite(mpq_denref(me), 1, st);
   }
   static
   void set_inf(mpq_ptr me, Int sign, const __mpq_struct& inv, initialized st=initialized::yes)
   {
      set_inf(me, sign, mpq_sgn(&inv), st);
   }
   static
   void set_inf(mpq_ptr me, Int sign, const __mpz_struct& inv, initialized st=initialized::yes)
   {
      set_inf(me, sign, mpz_sgn(&inv), st);
   }
   static
   void set_inf(mpq_ptr me, Int sign, const Integer& inv, initialized st=initialized::yes)
   {
      set_inf(me, sign, mpz_sgn(&inv), st);
   }
   static
   void inf_inv_sign(mpq_ptr me, long s)
   {
      Integer::inf_inv_sign(mpq_numref(me), s);
   }

   void canonicalize()
   {
      if (__builtin_expect(mpz_sgn(mpq_denref(this)), 1))
         mpq_canonicalize(this);
      else if (mpz_sgn(mpq_numref(this)))
         throw GMP::ZeroDivide();
      else
         throw GMP::NaN();
   }

   void canonicalize_sign()
   {
      if (mpz_sgn(mpq_denref(this))<0) {
         mpz_neg(mpq_numref(this), mpq_numref(this));
         mpz_neg(mpq_denref(this), mpq_denref(this));
      }
   }

   void canonicalize(num_proxy*)
   {
      if (__builtin_expect(isfinite(*this), 1))
         canonicalize();
      else
         mpz_set_ui(mpq_denref(this), 1);
   }

   void canonicalize(den_proxy*)
   {
      const bool set_to_finite=mpq_denref(this)->_mp_alloc;
      if (__builtin_expect(isfinite(*this), 1)) {
         if (__builtin_expect(set_to_finite, 1)) {
            canonicalize();
         } else {
            *this=0;
         }
      } else if (set_to_finite) {
         mpz_set_ui(mpq_denref(this), 1);
      } else {
         throw GMP::NaN();
      }
   }

   Integer* get_particle(num_proxy*) noexcept
   {
      return static_cast<Integer*>(mpq_numref(this));
   }
   const Integer* get_particle(const num_proxy*) const noexcept
   {
      return static_cast<const Integer*>(mpq_numref(this));
   }
   Integer* get_particle(den_proxy*) noexcept
   {
      return static_cast<Integer*>(mpq_denref(this));
   }
   const Integer* get_particle(const den_proxy*) const noexcept
   {
      return static_cast<const Integer*>(mpq_denref(this));
   }

   template <typename Src>
   void set_data(Src&& src, initialized st)
   {
      if (__builtin_expect(isfinite(src), 1)) {
         Integer::set_finite(mpq_numref(this), *mpq_numref(&src), st);
         Integer::set_finite(mpq_denref(this), *mpq_denref(&src), st);
      } else {
         set_inf(this, isinf(src), st);
      }
   }

   template <typename NumSrc, typename DenSrc>
   void set_data(NumSrc&& num, DenSrc&& den, initialized st)
   {
      if (__builtin_expect(isfinite(num), 1)) {
         if (__builtin_expect(isfinite(den), 1)) {
            Integer::set_finite(mpq_numref(this), num, st);
            Integer::set_finite(mpq_denref(this), den, st);
            canonicalize();
         } else {
            Integer::set_finite(mpq_numref(this), 0, st);
            Integer::set_finite(mpq_denref(this), 1, st);
         }
      } else if (isfinite(den)) {
         set_inf(this, isinf(num), den, st);
      } else {
         throw GMP::NaN();
      }
   }

   const Integer& num_ref() const noexcept
   {
      return *static_cast<const Integer*>(mpq_numref(this));
   }

   const Integer& den_ref() const noexcept
   {
      return *static_cast<const Integer*>(mpq_denref(this));
   }

   void parse(const char* s);

   void putstr(std::ios::fmtflags flags, char* buf, bool show_den) const;

   friend class Integer;
   friend class AccurateFloat;
   friend struct spec_object_traits<Rational>;
   template <bool, typename> friend class RationalParticle;
};

}
namespace std {

inline
void swap(pm::Rational& a, pm::Rational& b)
{
   a.swap(b);
}

template <bool is_numerator, typename TPart>
void swap(pm::RationalParticle<is_numerator, TPart>&& a, TPart& b)
{
   a.swap(b);
}

template <bool is_numerator2, typename TPart>
void swap(TPart& a, pm::RationalParticle<is_numerator2, TPart>&& b)
{
   b.swap(a);
}

template <bool is_numerator, bool is_numerator2, typename TPart>
void swap(pm::RationalParticle<is_numerator, TPart>&& a, pm::RationalParticle<is_numerator, TPart>&& b)
{
   a.swap(b);
}

template <>
class numeric_limits<pm::Rational>
   : public numeric_limits<pm::Integer> {
public:
   static const bool is_integer=false;

   static pm::Rational min() { return pm::Rational::infinity(-1); }
   static pm::Rational max() { return pm::Rational::infinity(1); }
   static pm::Rational infinity() { return pm::Rational::infinity(1); }
};

}
namespace pm {

inline
bool isfinite(const Rational& a) noexcept
{
   return mpq_numref(a.get_rep())->_mp_alloc;
}

inline
Int isinf(const Rational& a) noexcept
{
   return isfinite(a) ? 0 : mpq_numref(a.get_rep())->_mp_size;
}

inline
Int sign(const Rational& a) noexcept
{
   return mpq_sgn(a.get_rep());
}

template <bool is_numerator, typename TPart>
inline
void RationalParticle<is_numerator, TPart>::canonicalize()
{
   master->canonicalize(this);
}

template <bool is_numerator, typename TPart>
inline
TPart& RationalParticle<is_numerator, TPart>::particle() noexcept
{
   return *master->get_particle(this);
}

template <bool is_numerator, typename TPart>
inline
const TPart& RationalParticle<is_numerator, TPart>::particle() const noexcept
{
   return *master->get_particle(this);
}

inline
Integer floor(const Rational& a)
{
   return a.floor();
}

inline
Integer ceil(const Rational& a)
{
   return a.ceil();
}

inline
Integer trunc(const Rational& a)
{
   return a.trunc();
}

namespace operations {

template <>
struct cmp_scalar<Rational, Rational, void>
   : cmp_GMP_based<Rational> {};

} // end namespace operations

template <bool is_numerator, typename TPart>
struct object_traits<RationalParticle<is_numerator, TPart>>
   : object_traits<TPart> {
   using proxy_for = TPart;
   static constexpr bool is_temporary=true, is_persistent=false;
};

template <>
struct spec_object_traits<Rational>
   : spec_object_traits<is_scalar> {
   static
   bool is_zero(const Rational& a)
   {
      return a.is_zero();
   }

   static
   bool is_one(const Rational& a)
   {
      return a==1L;
   }

   static const Rational& zero();
   static const Rational& one();
};

// this is needed for various adapters transferring GMP values from third-parties
template <>
struct isomorphic_types<mpz_t, Rational>
   : std::true_type {};

template <>
struct isomorphic_types<mpq_t, Rational>
   : std::true_type {};

template <>
struct hash_func<Rational, is_scalar> : hash_func<MP_INT> {
protected:
   size_t impl(mpq_srcptr a) const
   {
      return hash_func<MP_INT>::impl(mpq_numref(a)) - hash_func<MP_INT>::impl(mpq_denref(a));
   }
public:
   size_t operator() (const Rational& a) const { return __builtin_expect(isfinite(a), 1) ? impl(a.get_rep()) : 0; }
};

template <>
struct algebraic_traits<long> {
   typedef Rational field_type;
};

template <>
struct algebraic_traits<Integer> {
   typedef Rational field_type;
};

template <>
struct algebraic_traits<Rational> {
   typedef Rational field_type;
};


inline
Integer::Integer(const Rational& b)
   : Integer(numerator_if_integral(b)) {}

inline
Integer::Integer(Rational&& b)
{
   if (!b.is_integral())
      throw GMP::BadCast("non-integral number");
   set_data(*mpq_numref(&b), initialized::no);
}

inline
Integer& Integer::operator= (const Rational& b)
{
   *this=numerator_if_integral(b);
   return *this;
}

inline
Integer& Integer::operator= (Rational&& b)
{
   if (!b.is_integral())
      throw GMP::BadCast("non-integral number");
   set_data(*mpq_numref(&b), initialized::yes);
   return *this;
}

Rational pow(const Rational& base, long exp);

}
namespace polymake {
   using pm::Rational;
}

#endif // POLYMAKE_RATIONAL_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
