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
   error(const std::string& what_arg) : std::domain_error(what_arg) {}
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

class TempInteger : public MP_INT {
protected:
   /// never instantiate this class: it is a pure masquerade
   TempInteger();
   ~TempInteger();
};

}

// forward declarations needed for friend and specializations

class Integer; class Rational; class AccurateFloat;

Integer gcd(const Integer& a, const Integer& b);
Integer gcd(const Integer& a, long b);

Integer lcm(const Integer& a, const Integer& b);
Integer lcm(const Integer& a, long b);

ExtGCD<Integer> ext_gcd(const Integer& a, const Integer& b);

Div<Integer> div(const Integer& a, const Integer& b);
Div<Integer> div(const Integer& a, long b);

Integer div_exact(const Integer& a, const Integer& b);
Integer div_exact(const Integer& a, long b);

int isfinite(const Integer& a);
int isinf(const Integer& a);

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

/** @class Integer
    @brief Wrapper class for @ref GMP "GMP"'s mpz_t type.
 */

class Integer {
protected:
   /// GMP's representation
   mpz_t rep;

public:

#if defined(__GMP_PLUSPLUS__)
   // convert to the GMP's own C++ wrapper
   mpz_class gmp() const
   {
      return mpz_class(rep);
   }
   
   // construct an Integer from the GMP's own C++ wrapper
   Integer(const mpz_class& i)
   {
      mpz_init_set(rep,i.get_mpz_t());
   }
#endif

   /// Initialize to 0.
   Integer() { mpz_init(rep); }

   Integer(const Integer& i)
   {
      if (__builtin_expect(isfinite(i),1))
         mpz_init_set(rep, i.rep);
      else
         _init_set_inf(rep, i.rep);
   }

   /// conversion
   Integer(long i)
   {
      mpz_init_set_si(rep, i);
   }

   /// conversion
   Integer(long long i)
   {
      mpz_init_set_si(rep, i);
   }

   /// conversion
   Integer(unsigned long i)
   {
      mpz_init_set_ui(rep, i);
   }

   /// conversion
   Integer(int i)
   {
      mpz_init_set_si(rep, i);
   }

   /// conversion
   Integer(double d)
   {
      const int is=isinf(d);
      if (__builtin_expect(is, 0))
         _init_set_inf(rep,is);
      else
         mpz_init_set_d(rep, d);
   }

   /// Recognizes automatically number base 10, 8, or 16.
   explicit Integer(const char* s)
   {
      mpz_init(rep);
      try {
         _set(s);
      }
      catch (const GMP::error&) {
         mpz_clear(rep);
         throw;
      }
   }

   explicit Integer(mpz_srcptr src)
   {
      mpz_init_set(rep, src);
   }

   /// take over the GMP structure without copying
   explicit Integer(GMP::TempInteger& tmp)
   {
      rep[0]=tmp;
   }
protected:
   static void _init_set_inf(mpz_ptr rep, int sign)
   {
      rep->_mp_alloc=0;
      rep->_mp_size=sign;
      rep->_mp_d=NULL;
   }
   static void _init_set_inf(mpz_ptr rep, mpz_srcptr b)
   {
      _init_set_inf(rep, b->_mp_size);
   }
   static void _init_set_inf(mpz_ptr rep, mpz_srcptr b, int inv)
   {
      _init_set_inf(rep, b->_mp_size<0 ? -inv : inv);
   }
   static void _set_inf(mpz_ptr rep, int sign)
   {
      mpz_clear(rep);
      _init_set_inf(rep,sign);
   }
   static void _set_inf(mpz_ptr rep, mpz_srcptr b)
   {
      _set_inf(rep, b->_mp_size);
   }
   static void _set_inf(mpz_ptr rep, mpz_srcptr b, int inv)
   {
      _set_inf(rep, b->_mp_size<0 ? -inv : inv);
   }
   static void _inf_inv_sign(mpz_ptr rep, long s, bool division=false)
   {
      if (s<0)
         rep->_mp_size*=-1;
      else if (s==0 && !division)
         throw GMP::NaN();
   }

   template <typename T>
   explicit Integer(maximal<T>, int s=1)
   {
      _init_set_inf(rep,s);
   }

   template <typename T>
   Integer(maximal<T>, mpz_srcptr b)
   {
      _init_set_inf(rep,b);
   }

   template <typename T>
   Integer(maximal<T>, mpz_srcptr b, int inv)
   {
      _init_set_inf(rep,b,inv);
   }
public:

   /// Performs division with rounding via truncation.
   explicit inline Integer(const Rational& b);

   explicit Integer(const AccurateFloat& b);

   /// Performs division with rounding via truncation.
   inline Integer& operator= (const Rational& b);

   Integer& operator= (const AccurateFloat& b);

   template <typename Arg>
   Integer(void (*f)(mpz_ptr,Arg), Arg a)
   {
      mpz_init(rep);
      f(rep,a);
   }

   template <typename Arg1, typename Arg2>
   Integer(void (*f)(mpz_ptr,Arg1,Arg2), Arg1 a, Arg2 b)
   {
      mpz_init(rep);
      f(rep,a,b);
   }

   template <typename Arg1, typename Arg2>
   Integer(Arg2 (*f)(mpz_ptr,Arg1,Arg2), Arg1 a, Arg2 b)
   {
      mpz_init(rep);
      f(rep,a,b);
   }

   struct Reserve {};

   /// Reserve space for n bits.
   Integer(size_t n, Reserve)
   {
      mpz_init2(rep, n);
   }

   static
   mpz_srcptr _tmp_negate(mpz_ptr temp, mpz_srcptr src)
   {
      temp->_mp_alloc =  src->_mp_alloc;
      temp->_mp_size  = -src->_mp_size;
      temp->_mp_d     =  src->_mp_d;
      return temp;
   }

   ~Integer()
   {
      mpz_clear(rep);
#if POLYMAKE_DEBUG
      POLYMAKE_DEBUG_METHOD(Integer,dump);
#endif
   }

   /// Assignment.
   Integer& operator= (const Integer& b)
   {
      const bool f1=isfinite(*this), f2=isfinite(b);
      if (__builtin_expect(f1 && f2, 1))
         mpz_set(rep, b.rep);
      else if (f2)
         mpz_init_set(rep, b.rep);
      else
         _set_inf(rep, b.rep);
      return *this;
   }

   /// Assignment with conversion.
   Integer& operator= (long b)
   {
      if (__builtin_expect(isfinite(*this),1))
         mpz_set_si(rep, b);
      else
         mpz_init_set_si(rep, b);
      return *this;
   }

   /// Assignment with conversion.
   Integer& operator= (unsigned long b)
   {
      if (__builtin_expect(isfinite(*this),1))
         mpz_set_ui(rep, b);
      else
         mpz_init_set_ui(rep, b);
      return *this;
   }

   /// Assignment with conversion.
   Integer& operator= (int b) { return operator=(long(b)); }

   /// Assignment with conversion.
   Integer& operator= (double d)
   {
      const bool f1=isfinite(*this);
      const int i2=isinf(d);
      if (__builtin_expect(f1 && !i2, 1))
         mpz_set_d(rep, d);
      else if (!i2)
         mpz_init_set_d(rep, d);
      else
         _set_inf(rep, i2);
      return *this;
   }

   /// Fill with bits coming from an open file.
   /// The current allocated size is preserved (see constructor with Reserve() argument).
   /// @retval true if filled successfully, false if eof or read error occured.
   bool fill_from_file(int fd);

protected:
   void _set(const char *s);

public:
   /// Recognizes automatically number base 10, 8, or 16.
   Integer& set(const char *s)
   {
      if (__builtin_expect(!isfinite(*this),0))
         mpz_init(rep);
      _set(s);
      return *this;
   }

   /// for the rare case of unwrapped GMP objects coexisting with us
   Integer& set(mpz_srcptr src)
   {
      if (__builtin_expect(isfinite(*this),1))
         mpz_set(rep, src);
      else
         mpz_init_set(rep, src);
      return *this;
   }

   /// Cast.
   double to_double() const
   {
      const int i=isinf(*this);
      if (__builtin_expect(i,0))
         return i*std::numeric_limits<double>::infinity();
      return mpz_get_d(rep);
   }

   /// Cast.
   long to_long() const
   {
      if (!mpz_fits_slong_p(rep) || !isfinite(*this))
         throw GMP::error("Integer: value too big");
      return mpz_get_si(rep);
   }

   /// Cast.
   int to_int() const
   {
      if (!mpz_fits_sint_p(rep) || !isfinite(*this))
         throw GMP::error("Integer: value too big");
      return mpz_get_si(rep);
   }

   /// Converts integer to string.
   std::string to_string(int base=10) const;

   bool non_zero() const
   {
      return mpz_sgn(rep);
   }

   /// Efficiently swapping two Integer objects.
   void swap(Integer& b) { mpz_swap(rep, b.rep); }

   /** Accelerated combination of copy constructor and destructor.
       Aimed to be used in container classes only! */
   friend void relocate(Integer* from, Integer* to)
   {
      to->rep[0] = from->rep[0];
   }

   /// Increment.
   Integer& operator++()
   {
      if (__builtin_expect(isfinite(*this),1)) mpz_add_ui(rep, rep, 1);
      return *this;
   }

   /// Decrement.
   Integer& operator--()
   {
      if (__builtin_expect(isfinite(*this),1)) mpz_sub_ui(rep, rep, 1);
      return *this;
   }

   /// In-place negation.
   Integer& negate()
   {
      rep[0]._mp_size*=-1;
      return *this;
   }

   /// Addition.
   Integer& operator+= (const Integer& b)
   {
      const bool f1=isfinite(*this), f2=isfinite(b);
      if (__builtin_expect(f1 && f2, 1))
         mpz_add(rep, rep, b.rep);
      else if (f1)
         _set_inf(rep, b.rep);
      else if (!f2 && isinf(*this)!=isinf(b))
         throw GMP::NaN();
      return *this;
   }

   /// Addition.
   Integer& operator+= (long b)
   {
      if (__builtin_expect(isfinite(*this),1)) {
         if (b>=0) mpz_add_ui(rep, rep, b);
         else mpz_sub_ui(rep, rep, -b);
      }
      return *this;
   }

   /// Subtraction.
   Integer& operator-= (const Integer& b)
   {
      const bool f1=isfinite(*this), f2=isfinite(b);
      if (__builtin_expect(f1 && f2, 1))
         mpz_sub(rep, rep, b.rep);
      else if (f1)
         _set_inf(rep, b.rep, -1);
      else if (!f2 && isinf(*this)==isinf(b))
         throw GMP::NaN();
      return *this;
   }

   /// Subtraction.
   Integer& operator-= (long b)
   {
      if (__builtin_expect(isfinite(*this),1)) {
         if (b>=0)
            mpz_sub_ui(rep, rep, b);
         else
            mpz_add_ui(rep, rep, -b);
      }
      return *this;
   }

   /// Multiplication.
   Integer& operator*= (const Integer& b)
   {
      const bool f1=isfinite(*this), f2=isfinite(b);
      if (__builtin_expect(f1 && f2, 1))
         mpz_mul(rep, rep, b.rep);
      else
         _inf_inv_sign(rep, mpz_sgn(b.rep));
      return *this;
   }

   /// Multiplication.
   Integer& operator*= (long b)
   {
      if (__builtin_expect(isfinite(*this),1))
         mpz_mul_si(rep, rep, b);
      else
         _inf_inv_sign(rep,b);
      return *this;
   }

   /// Division with rounding via truncation.
   Integer& operator/= (const Integer& b)
   {
      const bool f1=isfinite(*this), f2=isfinite(b);
      if (__builtin_expect(f2,1)) {
         const int s2=mpz_sgn(b.rep);
         if (__builtin_expect(f1,1)) {
            if (__builtin_expect(s2,1))
               mpz_tdiv_q(rep, rep, b.rep);
            else
               throw GMP::ZeroDivide();
         } else
            _inf_inv_sign(rep,s2,true);
      } else if (f1)
         mpz_set_ui(rep, 0);
      else
         throw GMP::NaN();
      return *this;
   }

   /// Division with rounding via truncation.
   Integer& operator/= (long b)
   {
      if (__builtin_expect(isfinite(*this),1)) {
         if (__builtin_expect(b,1)) {
            if (b>0)
               mpz_tdiv_q_ui(rep, rep, b);
            else {
               mpz_tdiv_q_ui(rep, rep, -b);
               negate();
            }
         } else
            throw GMP::ZeroDivide();
      } else
         _inf_inv_sign(rep,b,true);
      return *this;
   }

   /// b != infinity; but 0/0 allowed
   Integer& div_exact(const Integer& b)
   {
      const int s2=mpz_sgn(b.rep);
      if (__builtin_expect(isfinite(*this),1)) {
         if (__builtin_expect(s2,1))
            mpz_divexact(rep, rep, b.rep);
      } else
         _inf_inv_sign(rep,s2,true);
      return *this;
   }

   /// 0/0 allowed
   Integer& div_exact(long b)
   {
      if (__builtin_expect(isfinite(*this),1)) {
         if (__builtin_expect(b,1)) {
            if (b>0)
               mpz_divexact_ui(rep, rep, b);
            else {
               mpz_divexact_ui(rep, rep, -b);
               negate();
            }
         }
      } else
         _inf_inv_sign(rep,b,true);
      return *this;
   }

   /// Remainder of division.
   Integer& operator%= (const Integer& b)
   {
      const bool f1=isfinite(*this), f2=isfinite(b);
      if (__builtin_expect(!b.non_zero(), 0))
         throw GMP::ZeroDivide();
      if (__builtin_expect(f1 && f2, 1)) {
         mpz_tdiv_r(rep, rep, b.rep);
      } else if (f2)
         mpz_set_ui(rep, 0);
      else
         throw GMP::NaN();
      return *this;
   }

   /// Remainder of division.
   Integer& operator%= (long b)
   {
      if (__builtin_expect(!b,0))
         throw GMP::ZeroDivide();
      if (__builtin_expect(isfinite(*this),1))
         mpz_tdiv_r_ui(rep, rep, abs(b));
      else
         mpz_set_ui(rep, 0);
      return *this;
   }

   /// Multiply with 2**k.
   Integer& operator<<= (unsigned long k)
   {
      if (__builtin_expect(isfinite(*this),1))
         mpz_mul_2exp(rep, rep, k);
      return *this;
   }

   /// Divide through 2**k, truncate to zero.
   Integer& operator>>= (unsigned long k)
   {
      if (__builtin_expect(isfinite(*this),1))
         mpz_tdiv_q_2exp(rep, rep, k);
      return *this;
   }

   /// Addition.
   friend Integer operator+ (const Integer& a, const Integer& b)
   {
      const bool f1=isfinite(a), f2=isfinite(b);
      if (__builtin_expect(f1 && f2, 1))
         return Integer(mpz_add, a.rep, b.rep);
      if (f2) return a;
      if (!f1 && isinf(a)!=isinf(b))
         throw GMP::NaN();
      return b;
   }

   /// Addition.
   friend Integer operator+ (const Integer& a, long b)
   {
      if (__builtin_expect(isfinite(a),1))
         return Integer(b>=0 ? mpz_add_ui : mpz_sub_ui, a.rep, (unsigned long)abs(b));
      return a;
   }

   /// Subtraction.
   friend Integer operator- (const Integer& a, const Integer& b)
   {
      const bool f1=isfinite(a), f2=isfinite(b);
      if (__builtin_expect(f1 && f2, 1))
         return Integer(mpz_sub, a.rep, b.rep);
      if (f2) return a;
      if (!f1 && isinf(a)==isinf(b))
         throw GMP::NaN();
      return Integer(maximal<Integer>(), b.rep, -1);
   }

   /// Subtraction.
   friend Integer operator- (const Integer& a, long b)
   {
      if (__builtin_expect(isfinite(a),1))
         return Integer(b>=0 ? mpz_sub_ui : mpz_add_ui, a.rep, (unsigned long)abs(b));
      return a;
   }

   /// Subtraction.
   friend Integer operator- (long a, const Integer& b)
   {
      if (__builtin_expect(isfinite(b),1)) {
         mpz_t minus_b;
         return Integer(a>=0 ? mpz_add_ui : mpz_sub_ui,
                        _tmp_negate(minus_b,b.rep),  (unsigned long)abs(a));
      }
      return Integer(maximal<Integer>(), b.rep, -1);
   }

   /// Subtraction.
   friend Integer operator- (const Integer& a)
   {
      if (__builtin_expect(isfinite(a),1))
         return Integer(mpz_neg, a.rep);
      return Integer(maximal<Integer>(), a.rep, -1);
   }

   /// Multiplication.
   friend Integer operator* (const Integer& a, const Integer& b)
   {
      const bool f1=isfinite(a), f2=isfinite(b);
      if (__builtin_expect(f1 && f2, 1))
         return Integer(mpz_mul, a.rep, b.rep);
      const int s=mpz_sgn(a.rep)*mpz_sgn(b.rep);
      if (!s) throw GMP::NaN();
      return Integer(maximal<Integer>(), s);
   }

   /// Multiplication.
   friend Integer operator* (const Integer& a, long b)
   {
      if (__builtin_expect(isfinite(a),1))
         return Integer(mpz_mul_si, a.rep, b);
      if (!b) throw GMP::NaN();
      return Integer(maximal<Integer>(), a.rep, sign(b));
   }

   /// Division with rounding via truncation.
   friend Integer operator/ (const Integer& a, const Integer& b)
   {
      const bool f1=isfinite(a), f2=isfinite(b);
      if (__builtin_expect(f2,1)) {
         const int s2=mpz_sgn(b.rep);
         if (__builtin_expect(f1,1)) {
            if (__builtin_expect(s2,1))
               return Integer(mpz_tdiv_q, a.rep, b.rep);
            else
               throw GMP::ZeroDivide();
         }
         return Integer(maximal<Integer>(), a.rep, s2<0 ? -1 : 1);
      } else if (f1)
         return Integer();
      else
         throw GMP::NaN();
   }

   /// Division.
   friend Integer operator/ (const Integer& a, long b)
   {
      if (__builtin_expect(isfinite(a),1)) {
         if (__builtin_expect(b,1)) {
            if (b>0)
               return Integer(mpz_tdiv_q_ui, a.rep, (unsigned long)b);
            mpz_t minus_a;
            return Integer(mpz_tdiv_q_ui, _tmp_negate(minus_a,a.rep), (unsigned long)(-b));
         } else
            throw GMP::ZeroDivide();
      }
      return Integer(maximal<Integer>(), a.rep, b<0 ? -1 : 1);
   }

   /// Division.
   friend int operator/ (int a, const Integer& b)
   {
      if (__builtin_expect(!b.non_zero(), 0))
         throw GMP::ZeroDivide();
      return isfinite(b) && mpz_fits_sint_p(b.rep) ? a/mpz_get_si(b.rep) : 0;
   }

   /// Division.
   friend long operator/ (long a, const Integer& b)
   {
      if (__builtin_expect(!b.non_zero(), 0))
         throw GMP::ZeroDivide();
      return isfinite(b) && mpz_fits_slong_p(b.rep) ? a/mpz_get_si(b.rep) : 0;
   }

   /// Remainder of division.
   friend Integer operator% (const Integer& a, const Integer& b)
   {
      const bool f1=isfinite(a), f2=isfinite(b);
      if (__builtin_expect(!b.non_zero(), 0))
         throw GMP::ZeroDivide();
      if (__builtin_expect(f1 && f2, 1)) {
         return Integer(mpz_tdiv_r, a.rep, b.rep);
      }
      if (!f2)
         throw GMP::NaN();
      return Integer();
   }

   /// Remainder of division.
   friend long operator% (const Integer& a, long b)
   {
      if (__builtin_expect(!b, 0))
         throw GMP::ZeroDivide();
      if (__builtin_expect(isfinite(a),1)) {
         long r=mpz_tdiv_ui(a.rep, b);
         return mpz_sgn(a.rep)>=0 ? r : -r;
      }
      return 0;
   }

   /// Remainder of division.
   friend Integer operator% (int a, const Integer& b)
   {
      if (__builtin_expect(!b.non_zero(), 0))
         throw GMP::ZeroDivide();
      return isfinite(b) && mpz_fits_sint_p(b.rep) ? Integer(a%mpz_get_si(b.rep)) : b;
   }

   /// Remainder of division.
   friend Integer operator% (long a, const Integer& b)
   {
      if (__builtin_expect(!b.non_zero(), 0))
         throw GMP::ZeroDivide();
      return isfinite(b) && mpz_fits_slong_p(b.rep) ? Integer(a%mpz_get_si(b.rep)) : b;
   }

   /// Obtain both the quotient and remainder of a division
   friend
   Div<Integer> div(const Integer& a, const Integer& b);

   friend
   Div<Integer> div(const Integer& a, long b);

   /// Multiply with 2**k.
   friend Integer operator<< (const Integer& a, unsigned long k)
   {
      if (__builtin_expect(isfinite(a),1))
         return Integer(mpz_mul_2exp, a.rep, k);
      return a;
   }

   /// Divide through 2**k, truncate to 0.
   friend Integer operator>> (const Integer& a, unsigned long k)
   {
      if (__builtin_expect(isfinite(a),1))
         return Integer(mpz_tdiv_q_2exp, a.rep, k);
      return a;
   }

   /// Test for bits.
   bool bit(unsigned long i) const { return mpz_tstbit(rep, i); }

   /// Parity.
   bool odd() const { return isfinite(*this) && bit(0); }
   /// Parity.
   bool even() const { return !odd(); }

   friend int isfinite(const Integer& a);
   friend int isinf(const Integer& a);

   /// Comparison. The magnitude of the return value is arbitrary, only its sign is relevant.
   int compare(const Integer& b) const
   {
      const int i1=isinf(*this), i2=isinf(b);
      if (__builtin_expect(i1 | i2, 0))
         return i1-i2;
      return mpz_cmp(rep, b.rep);
   }

   /// Comparison.
   int compare(long b) const
   {
      const int i1=isinf(*this);
      if (__builtin_expect(i1,0))
         return i1;
      return mpz_cmp_si(rep, b);
   }

   /// Comparison.
   int compare(int b) const { return compare(long(b)); }

   /// Comparison.
   int compare(double b) const
   {
      const int i1=isinf(*this);
      if (__builtin_expect(i1,0))
         return i1-isinf(b);
      return mpz_cmp_d(rep, b);
   }

   typedef list comparable_with(int, long, double);

   /// Equality
   friend bool operator== (const Integer& a, long b)
   {
      return isfinite(a) && mpz_fits_slong_p(a.rep) && mpz_get_si(a.rep)==b;
   }

   /// Less than.
   friend bool operator< (const Integer& a, long b)
   {
      const int i1=isinf(a);
      if (__builtin_expect(i1,0)) return i1<0;
      return mpz_fits_slong_p(a.rep) ? mpz_get_si(a.rep)<b : mpz_sgn(a.rep)<0;
   }

   /// Greater than.
   friend bool operator> (const Integer& a, long b)
   {
      const int i1=isinf(a);
      if (__builtin_expect(i1,0)) return i1>0;
      return mpz_fits_slong_p(a.rep) ? mpz_get_si(a.rep)>b : mpz_sgn(a.rep)>0;
   }

   /// Equality of absolute values.
   friend bool abs_equal(const Integer& a, const Integer& b)
   {
      const bool f1=isfinite(a), f2=isfinite(b);
      if (__builtin_expect(f1 && f2, 1))
         return !mpz_cmpabs(a.rep, b.rep);
      return f1==f2;
   }

   /// Equality of absolute values.
   friend bool abs_equal(const Integer& a, long b)
   {
      return isfinite(a) && !mpz_cmpabs_ui(a.rep, abs(b));
   }

   /// Equality of absolute values.
   friend bool abs_equal(const Integer& a, double b)
   {
      if (__builtin_expect(isfinite(a),1))
         return !mpz_cmpabs_d(a.rep, b);
      return isinf(b);
   }

   /// Factorial.
   static Integer fac(long k)
   {
      if (k<0) throw std::runtime_error("fac not defined for negative values");
      return Integer(mpz_fac_ui, (unsigned long)k);
   }

   /// Power.
   static Integer pow(const Integer& a, unsigned long k)
   {
      if (__builtin_expect(isfinite(a),1))
         return Integer(mpz_pow_ui, a.rep, k);
      return Integer(maximal<Integer>(), k%2 ? isinf(a) : 1);
   }

   /// Power.
   static Integer pow(unsigned long a, unsigned long k)
   {
      return Integer(mpz_ui_pow_ui, a, k);
   }

   /// Square Root.
   friend Integer sqrt(const Integer& a)
   {
      if (__builtin_expect(isfinite(a),1))
         return Integer(mpz_sqrt, a.rep);
      return Integer(maximal<Integer>(), 1);
   }

   /// Absolute value.
   friend Integer abs(const Integer& a)
   {
      if (__builtin_expect(isfinite(a),1))
         return Integer(mpz_abs, a.rep);
      return Integer(maximal<Integer>(), 1);
   }

   /// Greatest common divisor.
   friend
   Integer gcd(const Integer& a, const Integer& b);

   /// Greatest common divisor.
   friend
   Integer gcd(const Integer& a, long b);

   /// Least common multiple.
   friend
   Integer lcm(const Integer& a, const Integer& b);

   friend
   Integer lcm(const Integer& a, long b);

   /// Extended gcd algorithm: g=a*p+b*q, a=m*g, b=n*g.
   friend
   ExtGCD<Integer> ext_gcd(const Integer& a, const Integer& b);

   friend
   Integer div_exact(const Integer& a, const Integer& b);

   friend
   Integer div_exact(const Integer& a, long b);

   /// Binomial coefficient.
   static
   Integer binom(const Integer& n, long k);

   /// Binomial coefficient.
   static
   Integer binom(long n, long k);

   /// @param allow_sign whether leading whitespaces and sign are expected
   void read(std::istream& is, bool allow_sign=true);

   /// Calculates the size of the buffer needed to store an ASCII representation of an Integer.
   size_t strsize(std::ios::fmtflags flags) const;

   /** Produces a printable representation of an Integer.
       @param buf buffer of size not less than the return value of strsize().
   */
   void putstr(std::ios::fmtflags flags, char *buf) const;

   friend class Rational;
   friend struct spec_object_traits<Integer>;
   template <typename> friend class std::numeric_limits;

   mpz_srcptr get_rep() const { return rep; }

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
   void dump() const { std::cerr << *this << std::flush; }
#endif
};

}
namespace std {

inline void swap(pm::Integer& i1, pm::Integer& i2) { i1.swap(i2); }

template <>
class numeric_limits<pm::Integer> : public numeric_limits<long> {
public:
   static const bool has_infinity=true;
   static pm::Integer min() throw() { return pm::Integer(pm::maximal<pm::Integer>(),-1); }
   static pm::Integer infinity() throw() { return pm::Integer(pm::maximal<pm::Integer>()); }
   static pm::Integer max() throw() { return pm::Integer(pm::maximal<pm::Integer>()); }
   static const int digits=INT_MAX;
   static const int digits10=INT_MAX;
   static const bool is_bounded=false;
};

}
namespace pm {

inline Integer operator+ (const Integer& a) { return a; }

inline const Integer operator++ (Integer& a, int) { Integer copy(a); ++a; return copy; }
inline const Integer operator-- (Integer& a, int) { Integer copy(a); --a; return copy; }

inline Integer operator+ (long a, const Integer& b) { return b+a; }
inline Integer operator+ (const Integer& a, int b) { return a+long(b); }
inline Integer operator+ (int a, const Integer& b) { return b+long(a); }

inline Integer operator- (const Integer& a, int b) { return a-long(b); }
inline Integer operator- (int a, const Integer& b) { return long(a)-b; }

inline Integer operator* (long a, const Integer& b) { return b*a; }
inline Integer operator* (const Integer& a, int b) { return a*long(b); }
inline Integer operator* (int a, const Integer& b) { return b*long(a); }

inline Integer operator/ (const Integer& a, int b) { return a/long(b); }
inline Integer operator% (const Integer& a, int b) { return a%long(b); }

inline Integer operator<< (const Integer& a, unsigned int k)
{
   return a << static_cast<unsigned long>(k);
}

inline Integer operator>> (const Integer& a, unsigned int k)
{
   return a >> static_cast<unsigned long>(k);
}

inline Integer operator<< (const Integer& a, long k)
{
   if (k<0) return a >> static_cast<unsigned long>(-k);
   return a << static_cast<unsigned long>(k);
}

inline Integer operator>> (const Integer& a, long k)
{
   if (k<0) return a << static_cast<unsigned long>(-k);
   return a >> static_cast<unsigned long>(k);
}

inline Integer operator<< (const Integer& a, int k) { return a << long(k); }
inline Integer operator>> (const Integer& a, int k) { return a >> long(k); }

inline bool operator== (const Integer& a, const Integer& b) { return a.compare(b)==0; }
inline bool operator!= (const Integer& a, const Integer& b) { return a.compare(b)!=0; }
inline bool operator< (const Integer& a, const Integer& b) { return a.compare(b)<0; }
inline bool operator> (const Integer& a, const Integer& b) { return a.compare(b)>0; }
inline bool operator<= (const Integer& a, const Integer& b) { return a.compare(b)<=0; }
inline bool operator>= (const Integer& a, const Integer& b) { return a.compare(b)>=0; }

namespace GMP {
   inline bool operator== (const TempInteger& a, const TempInteger& b) { return mpz_cmp(&a,&b)==0; }
   inline bool operator!= (const TempInteger& a, const TempInteger& b) { return mpz_cmp(&a,&b)!=0; }
   inline bool operator< (const TempInteger& a, const TempInteger& b) { return mpz_cmp(&a,&b)<0; }
   inline bool operator> (const TempInteger& a, const TempInteger& b) { return mpz_cmp(&a,&b)>0; }
   inline bool operator<= (const TempInteger& a, const TempInteger& b) { return mpz_cmp(&a,&b)<=0; }
   inline bool operator>= (const TempInteger& a, const TempInteger& b) { return mpz_cmp(&a,&b)>=0; }
}

inline bool operator== (long a, const Integer& b) { return b==a; }
inline bool operator== (const Integer& a, int b) { return a==long(b); }
inline bool operator== (int a, const Integer& b) { return b==long(a); }
inline bool operator== (const Integer& a, double b) { return !a.compare(b); }
inline bool operator== (double a, const Integer& b) { return !b.compare(a); }
inline bool abs_equal(long a, const Integer& b) { return abs_equal(b,a); }
inline bool abs_equal(double a, const Integer& b) { return abs_equal(b,a); }
inline bool abs_equal(const Integer& a, int b) { return abs_equal(a,long(b)); }
inline bool abs_equal(int a, const Integer& b) { return abs_equal(b,long(a)); }

inline bool operator!= (const Integer& a, long b) { return !(a==b); }
inline bool operator!= (const Integer& a, int b) { return !(a==long(b)); }
inline bool operator!= (long a, const Integer& b) { return !(b==a); }
inline bool operator!= (int a, const Integer& b) { return !(b==long(a)); }
inline bool operator!= (const Integer& a, double b) { return !(a==b); }
inline bool operator!= (double a, const Integer& b) { return !(b==a); }

inline bool operator< (const Integer& a, int b) { return a<long(b); }
inline bool operator< (long a, const Integer& b) { return b>a; }
inline bool operator< (int a, const Integer& b) { return b>long(a); }
inline bool operator< (const Integer& a, double b) { return a.compare(b)<0; }
inline bool operator< (double a, const Integer& b) { return b.compare(a)>0; }

inline bool operator> (const Integer& a, int b) { return a>long(b); }
inline bool operator> (long a, const Integer& b) { return b<a; }
inline bool operator> (int a, const Integer& b) { return b<long(a); }
inline bool operator> (const Integer& a, double b) { return a.compare(b)>0; }
inline bool operator> (double a, const Integer& b) { return b.compare(a)<0; }

inline bool operator<= (const Integer& a, long b) { return !(a>b); }
inline bool operator<= (const Integer& a, int b) { return !(a>long(b)); }
inline bool operator<= (long a, const Integer& b) { return !(b<a); }
inline bool operator<= (int a, const Integer& b) { return !(b<long(a)); }
inline bool operator<= (const Integer& a, double b) { return !(a>b); }
inline bool operator<= (double a, const Integer& b) { return !(b<a); }

inline bool operator>= (const Integer& a, long b) { return !(a<b); }
inline bool operator>= (const Integer& a, int b) { return !(a<long(b)); }
inline bool operator>= (long a, const Integer& b) { return !(b>a); }
inline bool operator>= (int a, const Integer& b) { return !(b>long(a)); }
inline bool operator>= (const Integer& a, double b) { return !(a<b); }
inline bool operator>= (double a, const Integer& b) { return !(b>a); }

template <> inline
cmp_value sign(const Integer& a) { return sign(a.get_rep()->_mp_size); }

/// Returns a positive number if not equal to infinity, otherwise zero.
inline
int isfinite(const Integer& a)
{
   return a.rep[0]._mp_alloc;
}

/// Returns the sign if equal to +/-infinity, otherwise zero.
inline
int isinf(const Integer& a)
{
   return isfinite(a) ? 0 : a.rep[0]._mp_size;
}

inline
Integer gcd(const Integer& a, const Integer& b)
{
   const bool f1=isfinite(a), f2=isfinite(b);
   if (__builtin_expect(f1 && f2, 1))
      return Integer(mpz_gcd, a.rep, b.rep);
   if (f1) return a;
   return b;
}

inline
Integer gcd(const Integer& a, long b)
{
   if (__builtin_expect(isfinite(a),1))
      return Integer(mpz_gcd_ui, a.rep, (unsigned long)b);
   return b;
}

inline
Integer gcd(long a, const Integer& b) { return gcd(b,a); }

inline
Integer lcm(const Integer& a, const Integer& b)
{
   const bool f1=isfinite(a), f2=isfinite(b);
   if (__builtin_expect(f1 && f2, 1))
      return Integer(mpz_lcm, a.rep, b.rep);
   return Integer(maximal<Integer>(), 1);
}

inline
Integer lcm(const Integer& a, long b)
{
   if (__builtin_expect(isfinite(a),1))
      return Integer(mpz_lcm_ui, a.rep, (unsigned long)b);
   return Integer(maximal<Integer>(), 1);
}

inline
Integer lcm(long a, const Integer& b) { return lcm(b,a); }

inline
ExtGCD<Integer> ext_gcd(const Integer& a, const Integer& b)
{
   ExtGCD<Integer> res;
   const bool f1=isfinite(a), f2=isfinite(b);
   if (__builtin_expect(f1 && f2, 1)) {
      mpz_gcdext(res.g.rep, res.p.rep, res.q.rep, a.rep, b.rep);
      mpz_divexact(res.k1.rep, a.rep, res.g.rep);
      mpz_divexact(res.k2.rep, b.rep, res.g.rep);
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
   const bool f1=isfinite(a), f2=isfinite(b);
   if (__builtin_expect(!b.non_zero(), 0))
      throw GMP::ZeroDivide();
   if (__builtin_expect(f1 && f2, 1))
      mpz_tdiv_qr(res.quot.rep, res.rem.rep, a.rep, b.rep);
   else if (f1)
      throw GMP::NaN();
   else if (f2)
      Integer::_set_inf(res.quot.rep, b.rep), res.rem=0;
   else
      throw GMP::NaN();
   return res;
}

inline
Div<Integer> div(const Integer& a, long b)
{
   Div<Integer> res;
   const bool f1=isfinite(a);
   if (__builtin_expect(b==0, 0))
      throw GMP::ZeroDivide();
   if (__builtin_expect(f1, 1))
      mpz_tdiv_qr_ui(res.quot.rep, res.rem.rep, a.rep, b);
   else
      Integer::_set_inf(res.quot.rep, b), res.rem=0;
   return res;
}

inline
Integer div_exact(const Integer& a, const Integer& b)
{
   if (__builtin_expect(isfinite(a), 1)) {
      if (__builtin_expect(!b.non_zero(), 0)) return a;     // allow 0/0
      return Integer(mpz_divexact, a.rep, b.rep);
   }
   return Integer(maximal<Integer>(), mpz_sgn(a.rep)*mpz_sgn(b.rep));
}

inline
Integer div_exact(const Integer& a, long b)
{
   if (__builtin_expect(isfinite(a), 1)) {
      if (__builtin_expect(!b,0)) return a;     // allow 0/0
      if (b>0)
         return Integer(mpz_divexact_ui, a.rep, (unsigned long)b);
      mpz_t minus_a;
      return Integer(mpz_divexact_ui, a._tmp_negate(minus_a,a.rep), (unsigned long)(-b));
   }
   return Integer(maximal<Integer>(), mpz_sgn(a.rep)*pm::sign(b));
}

/// Logarithm (rounded down).
inline
int log2_floor(const Integer& a)
{
   if (__builtin_expect(!isfinite(a), 0)) throw GMP::NaN();
   int n=mpz_size(a.get_rep())-1;
   return n>=0 ? n*mp_bits_per_limb + log2_floor(mpz_getlimbn(a.get_rep(),n)) : 0;
}

/// Logarithm (rounded up). 
inline
int log2_ceil(const Integer& a)
{
   if (__builtin_expect(!isfinite(a),0)) throw GMP::NaN();
   int n=mpz_size(a.get_rep())-1;
   return n>=0 ? n*mp_bits_per_limb + log2_ceil(mpz_getlimbn(a.get_rep(),n)) : 0;
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
      return sign(a.compare(b));
   }

   template <typename T2>
   typename enable_if<cmp_value, (pass_by_reference<T2>::value && list_contains<typename T::comparable_with, T2>::value)>::type
   operator() (const T& a, const T2& b) const
   {
      return sign(a.compare(b));
   }

   template <typename T2>
   typename enable_if<cmp_value, (pass_by_value<T2>::value && list_contains<typename T::comparable_with, T2>::value)>::type
   operator() (const T& a, const T2 b) const
   {
      return sign(a.compare(b));
   }

   template <typename T2>
   typename enable_if<cmp_value, (!list_contains<typename T::comparable_with, T2>::value && list_contains<typename T2::comparable_with, T>::value)>::type
   operator() (const T& a, const T2& b) const
   {
      return sign(-b.compare(a));
   }

   template <typename T2>
   typename enable_if<cmp_value, (pass_by_reference<T2>::value && list_contains<typename T::comparable_with, T2>::value)>::type
   operator() (const T2& a, const T& b) const
   {
      return sign(-b.compare(a));
   }

   template <typename T2>
   typename enable_if<cmp_value, (pass_by_value<T2>::value && list_contains<typename T::comparable_with, T2>::value)>::type
   operator() (const T2 a, const T& b) const
   {
      return sign(-b.compare(a));
   }

   template <typename T2>
   typename enable_if<cmp_value, (!list_contains<typename T::comparable_with, T2>::value && list_contains<typename T2::comparable_with, T>::value)>::type
   operator() (const T2& a, const T& b) const
   {
      return sign(a.compare(b));
   }
};

template <>
struct cmp_scalar<Integer, Integer, true> : cmp_GMP_based<Integer> {};

} // end namespace operations

template <>
class conv<Integer, int> {
public:
   typedef Integer argument_type;
   typedef int result_type;
   result_type operator() (const Integer& a) const { return a.to_int(); }
};

template <>
class conv<Integer, long> {
public:
   typedef Integer argument_type;
   typedef long result_type;
   result_type operator() (const Integer& a) const { return a.to_long(); }
};

template <>
class conv<Integer, double> {
public:
   typedef Integer argument_type;
   typedef double result_type;
   result_type operator() (const Integer& a) const { return a.to_double(); }
};

template <>
class conv<Integer, std::string> {
public:
   typedef Integer argument_type;
   typedef std::string result_type;
   result_type operator() (const Integer& a) const { return a.to_string(); }
};

template <>
struct spec_object_traits<GMP::TempInteger> : spec_object_traits<is_scalar> {};

template <>
class conv<GMP::TempInteger, Integer> : conv_by_cast<GMP::TempInteger&, Integer> {};

template <>
struct spec_object_traits<Integer> : spec_object_traits<is_scalar> {
   static
   bool is_zero(const Integer& a)
   {
      return !a.non_zero();
   }

   static
   bool is_one(const Integer& a)
   {
      return a==1L;
   }

   static const Integer& zero();
   static const Integer& one();
};

template <> struct hash_func<MP_INT, is_opaque> {
protected:
   size_t _do(mpz_srcptr a) const
   {
      size_t result=0;
      for (int i=0, n=mpz_size(a); i<n; ++i)
         (result <<= 1) ^= mpz_getlimbn(a, i);
      return result;
   }
public:
   size_t operator() (const MP_INT& a) const { return _do(&a); }
};

template <>
struct hash_func<Integer, is_scalar> : hash_func<MP_INT>
{
   size_t operator() (const Integer& a) const
   {
      return __builtin_expect(isfinite(a), 1) ? _do(a.get_rep()) : 0;
   }
};

template <>
struct hash_func<GMP::TempInteger, is_scalar> : hash_func<MP_INT>
{
   size_t operator() (const GMP::TempInteger& a) const { return hash_func<MP_INT>::operator()(reinterpret_cast<const MP_INT&>(a)); }
};

namespace GMP {

inline
std::ostream& operator<< (std::ostream& os, const TempInteger& a)
{
   return os << reinterpret_cast<const Integer&>(a);
}

}
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
