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

#ifndef POLYMAKE_ACCURATERATIONAL_H
#define POLYMAKE_ACCURATERATIONAL_H

#include "polymake/Rational.h"
#include <mpfr.h>
#include <sstream>

namespace pm {

/// minimalistic wrapper for MPFR numbers
class AccurateFloat {
   friend class Integer;  friend class Rational;
   template <typename> friend class std::numeric_limits;
protected:
   mpfr_t rep;
public:
   AccurateFloat()
   {
      mpfr_init_set_ui(rep, 0, MPFR_RNDZ);
   }

   AccurateFloat(const AccurateFloat& a)
   {
      mpfr_init_set(rep, a.rep, MPFR_RNDN);
   }

   explicit AccurateFloat(const Integer& a)
   {
      if (__builtin_expect(isfinite(a),1)) {
         mpfr_init_set_z(rep, a.get_rep(), MPFR_RNDZ);
      } else {
         mpfr_init(rep);
         mpfr_set_inf(rep, sign(a));
      }
   }

   explicit AccurateFloat(const Rational& a)
   {
      if (__builtin_expect(isfinite(a),1)) {
         mpfr_init_set_q(rep, a.get_rep(), MPFR_RNDN);
      } else {
         mpfr_init(rep);
         mpfr_set_inf(rep, sign(a));
      }
   }

   explicit AccurateFloat(long a)
   {
      mpfr_init_set_si(rep, a, MPFR_RNDZ);
   }

   explicit AccurateFloat(int a)
   {
      mpfr_init_set_si(rep, a, MPFR_RNDZ);
   }

   explicit AccurateFloat(double a)
   {
      mpfr_init_set_d(rep, a, MPFR_RNDN);
   }

protected:
   template <typename T>
   explicit AccurateFloat(maximal<T>, int s=1)
   {
      mpfr_init(rep);
      mpfr_set_inf(rep,s);
   }
public:
   explicit AccurateFloat(int (*f)(mpfr_ptr,mpfr_rnd_t), mpfr_rnd_t r)
   {
      mpfr_init(rep);
      f(rep,r);
   }
   explicit AccurateFloat(int (*f)(mpfr_ptr,mpfr_srcptr), mpfr_srcptr a)
   {
      mpfr_init(rep);
      f(rep,a);
   }
   template <typename Arg>
   explicit AccurateFloat(int (*f)(mpfr_ptr,Arg,mpfr_rnd_t), Arg a, mpfr_rnd_t r)
   {
      mpfr_init(rep);
      f(rep,a,r);
   }
   template <typename Arg1, typename Arg2>
   explicit AccurateFloat(int (*f)(mpfr_ptr,Arg1,Arg2,mpfr_rnd_t), Arg1 a, Arg2 b, mpfr_rnd_t r)
   {
      mpfr_init(rep);
      f(rep,a,b,r);
   }

   template <bool inv> struct Add {};
   struct Mul {};  struct Div {};

   // for Float+/-Rational, Float+/-Integer
   template <typename Arg2, bool inv2>
   explicit AccurateFloat(int (*f)(mpfr_ptr,mpfr_srcptr,Arg2,mpfr_rnd_t), mpfr_srcptr a, Arg2 b, mpfr_rnd_t r, int s2, Add<inv2>)
   {
      mpfr_init(rep);
      if (__builtin_expect(s2,0)) {
         const int s1=mpfr_inf_p(a) ? mpfr_sgn(a) : 0;
         if (inv2) s2=-s2;
         if (!s1 || s2 == s1)
            mpfr_set_inf(rep, s2);  // else *this remains NaN
      } else {
         f(rep,a,b,r);
      }
   }

   // for Float*Rational, Float*Integer
   template <typename Arg2>
   explicit AccurateFloat(int (*f)(mpfr_ptr,mpfr_srcptr,Arg2,mpfr_rnd_t), mpfr_srcptr a, Arg2 b, mpfr_rnd_t r, int s2, Mul)
   {
      mpfr_init(rep);
      if (__builtin_expect(s2,0)) {
         if (!mpfr_zero_p(a) && !mpfr_nan_p(a))
            mpfr_set_inf(rep, mpfr_sgn(a)*s2);
      } else {
         f(rep,a,b,r);
      }
   }

   // for Float/Rational, Float/Integer
   template <typename Arg2>
   explicit AccurateFloat(int (*f)(mpfr_ptr,mpfr_srcptr,Arg2,mpfr_rnd_t), mpfr_srcptr a, Arg2 b, mpfr_rnd_t r, int s2, Div)
   {
      mpfr_init(rep);
      if (__builtin_expect(s2,0)) {
         if (!mpfr_inf_p(a) && !mpfr_nan_p(a))
            mpfr_set_zero(rep, mpfr_sgn(a)*s2);
      } else {
         f(rep,a,b,r);
      }
   }

   static
   mpfr_srcptr _tmp_negate(mpfr_ptr temp, mpfr_srcptr src)
   {
      temp->_mpfr_prec =  src->_mpfr_prec;
      temp->_mpfr_sign = -src->_mpfr_sign;
      temp->_mpfr_exp  =  src->_mpfr_exp;
      temp->_mpfr_d    =  src->_mpfr_d;
      return temp;
   }

   ~AccurateFloat()
   {
      mpfr_clear(rep);
#if POLYMAKE_DEBUG
      POLYMAKE_DEBUG_METHOD(AccurateFloat,dump);
#endif
   }

   mpfr_srcptr get_rep() const { return rep; }

   friend int isfinite(const AccurateFloat& a)
   {
      return mpfr_number_p(a.rep);
   }

   friend int isinf(const AccurateFloat& a)
   {
      return mpfr_inf_p(a.rep) ? mpfr_sgn(a.rep) : 0;
   }

   AccurateFloat& operator= (const AccurateFloat& a)
   {
      mpfr_set(rep, a.rep, MPFR_RNDN);
      return *this;
   }

   AccurateFloat& set(mpfr_srcptr src)
   {
      mpfr_set(rep, src, MPFR_RNDN);
      return *this;
   }

   AccurateFloat& set_random(gmp_randstate_t rnd)
   {
      mpfr_urandom(rep, rnd, MPFR_RNDZ);
      return *this;
   }

   AccurateFloat& operator= (const Rational& b)
   {
      const int s2=isinf(b);
      if (__builtin_expect(s2,0))
         mpfr_set_inf(rep,s2);
      else
         mpfr_set_q(rep, b.get_rep(), MPFR_RNDN);
      return *this;
   }

   AccurateFloat& operator= (const Integer& b)
   {
      const int s2=isinf(b);
      if (__builtin_expect(s2,0))
         mpfr_set_inf(rep,s2);
      else
         mpfr_set_z(rep, b.get_rep(), MPFR_RNDZ);
      return *this;
   }

   AccurateFloat& operator= (long b)
   {
      mpfr_set_si(rep, b, MPFR_RNDZ);
      return *this;
   }

   AccurateFloat& operator= (int b)
   {
      mpfr_set_si(rep, b, MPFR_RNDZ);
      return *this;
   }

   AccurateFloat& operator= (double b)
   {
      mpfr_set_d(rep, b, MPFR_RNDN);
      return *this;
   }

   void swap(AccurateFloat& b) { mpfr_swap(rep, b.rep); }

   friend void relocate(AccurateFloat* from, AccurateFloat* to)
   {
      to->rep[0] = from->rep[0];
   }

   AccurateFloat& operator++ ()
   {
      mpfr_add_si(rep, rep, 1, MPFR_RNDZ);
      return *this;
   }

   AccurateFloat& operator-- ()
   {
      mpfr_sub_si(rep, rep, 1, MPFR_RNDZ);
      return *this;
   }

   AccurateFloat& negate()
   {
      mpfr_neg(rep, rep, MPFR_RNDN);
      return *this;
   }

   AccurateFloat& operator+= (const AccurateFloat& b)
   {
      mpfr_add(rep, rep, b.rep, MPFR_RNDN);
      return *this;
   }

   AccurateFloat& operator+= (long b)
   {
      mpfr_add_si(rep, rep, b, MPFR_RNDN);
      return *this;
   }

   AccurateFloat& operator+= (double b)
   {
      mpfr_add_d(rep, rep, b, MPFR_RNDN);
      return *this;
   }
protected:
   bool assign_add_inf(int s2, int inv=1)
   {
      if (__builtin_expect(s2,0)) {
         const int s1=isinf(*this);
         s2*=inv;
         if (s1) {
            if (s2 != s1) mpfr_set_nan(rep);
         } else {
            mpfr_set_inf(rep, s2);
         }
         return true;
      }
      return false;
   }

   bool assign_mul_inf(int s2)
   {
      if (__builtin_expect(s2,0)) {
         if (mpfr_zero_p(rep))
            mpfr_set_nan(rep);
         else if (!mpfr_nan_p(rep))
            mpfr_set_inf(rep, mpfr_sgn(rep)*s2);
         return true;
      }
      return false;
   }

   bool assign_div_inf(int s2)
   {
      if (__builtin_expect(s2,0)) {
         if (mpfr_inf_p(rep))
            mpfr_set_nan(rep);
         else if (!mpfr_nan_p(rep))
            mpfr_set_zero(rep, mpfr_sgn(rep)*s2);
         return true;
      }
      return false;
   }
public:
   AccurateFloat& operator+= (const Integer& b)
   {
      if (!assign_add_inf(isinf(b)))
         mpfr_add_z(rep, rep, b.get_rep(), MPFR_RNDN);
      return *this;
   }

   AccurateFloat& operator+= (const Rational& b)
   {
      if (!assign_add_inf(isinf(b)))
         mpfr_add_q(rep, rep, b.get_rep(), MPFR_RNDN);
      return *this;
   }

   AccurateFloat& operator-= (const AccurateFloat& b)
   {
      mpfr_sub(rep, rep, b.rep, MPFR_RNDN);
      return *this;
   }

   AccurateFloat& operator-= (long b)
   {
      mpfr_sub_si(rep, rep, b, MPFR_RNDN);
      return *this;
   }

   AccurateFloat& operator-= (double b)
   {
      mpfr_sub_d(rep, rep, b, MPFR_RNDN);
      return *this;
   }

   AccurateFloat& operator-= (const Integer& b)
   {
      if (!assign_add_inf(isinf(b), -1))
         mpfr_sub_z(rep, rep, b.get_rep(), MPFR_RNDN);
      return *this;
   }

   AccurateFloat& operator-= (const Rational& b)
   {
      if (!assign_add_inf(isinf(b), -1))
         mpfr_sub_q(rep, rep, b.get_rep(), MPFR_RNDN);
      return *this;
   }

   AccurateFloat& operator*= (const AccurateFloat& b)
   {
      mpfr_mul(rep, rep, b.rep, MPFR_RNDN);
      return *this;
   }

   AccurateFloat& operator*= (long b)
   {
      mpfr_mul_si(rep, rep, b, MPFR_RNDN);
      return *this;
   }

   AccurateFloat& operator*= (double b)
   {
      mpfr_mul_d(rep, rep, b, MPFR_RNDN);
      return *this;
   }

   AccurateFloat& operator*= (const Integer& b)
   {
      if (!assign_mul_inf(isinf(b)))
         mpfr_mul_z(rep, rep, b.get_rep(), MPFR_RNDN);
      return *this;
   }

   AccurateFloat& operator*= (const Rational& b)
   {
      if (!assign_mul_inf(isinf(b)))
         mpfr_mul_q(rep, rep, b.get_rep(), MPFR_RNDN);
      return *this;
   }

   AccurateFloat& operator/= (const AccurateFloat& b)
   {
      mpfr_div(rep, rep, b.rep, MPFR_RNDN);
      return *this;
   }

   AccurateFloat& operator/= (long b)
   {
      mpfr_div_si(rep, rep, b, MPFR_RNDN);
      return *this;
   }

   AccurateFloat& operator/= (double b)
   {
      mpfr_div_d(rep, rep, b, MPFR_RNDN);
      return *this;
   }

   AccurateFloat& operator/= (const Integer& b)
   {
      if (!assign_div_inf(isinf(b)))
         mpfr_div_z(rep, rep, b.get_rep(), MPFR_RNDN);
      return *this;
   }

   AccurateFloat& operator/= (const Rational& b)
   {
      if (!assign_div_inf(isinf(b)))
         mpfr_div_q(rep, rep, b.get_rep(), MPFR_RNDN);
      return *this;
   }

   AccurateFloat& operator<<= (unsigned long k)
   {
      mpfr_mul_2ui(rep, rep, k, MPFR_RNDZ);
      return *this;
   }

   AccurateFloat& operator>>= (unsigned long k)
   {
      mpfr_div_2ui(rep, rep, k, MPFR_RNDZ);
      return *this;
   }

   friend AccurateFloat operator- (const AccurateFloat& a)
   {
      return AccurateFloat(mpfr_neg, a.rep, MPFR_RNDN);
   }

   friend AccurateFloat operator+ (const AccurateFloat& a, const AccurateFloat& b)
   {
      return AccurateFloat(mpfr_add, a.rep, b.rep, MPFR_RNDN);
   }

   friend AccurateFloat operator+ (const AccurateFloat& a, long b)
   {
      return AccurateFloat(mpfr_add_si, a.rep, b, MPFR_RNDN);
   }

   friend AccurateFloat operator+ (const AccurateFloat& a, double b)
   {
      return AccurateFloat(mpfr_add_d, a.rep, b, MPFR_RNDN);
   }

   friend AccurateFloat operator+ (const AccurateFloat& a, const Integer& b)
   {
      return AccurateFloat(mpfr_add_z, a.rep, b.get_rep(), MPFR_RNDN, isinf(b), Add<false>());
   }

   friend AccurateFloat operator+ (const AccurateFloat& a, const Rational& b)
   {
      return AccurateFloat(mpfr_add_q, a.rep, b.get_rep(), MPFR_RNDN, isinf(b), Add<false>());
   }

   friend AccurateFloat operator- (const AccurateFloat& a, const AccurateFloat& b)
   {
      return AccurateFloat(mpfr_sub, a.rep, b.rep, MPFR_RNDN);
   }

   friend AccurateFloat operator- (const AccurateFloat& a, long b)
   {
      return AccurateFloat(mpfr_sub_si, a.rep, b, MPFR_RNDN);
   }

   friend AccurateFloat operator- (const AccurateFloat& a, double b)
   {
      return AccurateFloat(mpfr_sub_d, a.rep, b, MPFR_RNDN);
   }

   friend AccurateFloat operator- (const AccurateFloat& a, const Integer& b)
   {
      return AccurateFloat(mpfr_sub_z, a.rep, b.get_rep(), MPFR_RNDN, isinf(b), Add<true>());
   }

   friend AccurateFloat operator- (const AccurateFloat& a, const Rational& b)
   {
      return AccurateFloat(mpfr_sub_q, a.rep, b.get_rep(), MPFR_RNDN, isinf(b), Add<true>());
   }

   friend AccurateFloat operator- (long a, const AccurateFloat& b)
   {
      return AccurateFloat(mpfr_si_sub, a, b.rep, MPFR_RNDN);
   }

   friend AccurateFloat operator- (double a, const AccurateFloat& b)
   {
      return AccurateFloat(mpfr_d_sub, a, b.rep, MPFR_RNDN);
   }

   friend AccurateFloat operator- (const Integer& a, const AccurateFloat& b)
   {
      mpfr_t temp;
      return AccurateFloat(mpfr_add_z, _tmp_negate(temp, b.rep), a.get_rep(), MPFR_RNDN, isinf(a), Add<false>());
   }

   friend AccurateFloat operator- (const Rational& a, const AccurateFloat& b)
   {
      mpfr_t temp;
      return AccurateFloat(mpfr_add_q, _tmp_negate(temp, b.rep), a.get_rep(), MPFR_RNDN, isinf(a), Add<false>());
   }

   friend AccurateFloat operator* (const AccurateFloat& a, const AccurateFloat& b)
   {
      return AccurateFloat(mpfr_mul, a.rep, b.rep, MPFR_RNDN);
   }

   friend AccurateFloat operator* (const AccurateFloat& a, long b)
   {
      return AccurateFloat(mpfr_mul_si, a.rep, b, MPFR_RNDN);
   }

   friend AccurateFloat operator* (const AccurateFloat& a, double b)
   {
      return AccurateFloat(mpfr_mul_d, a.rep, b, MPFR_RNDN);
   }

   friend AccurateFloat operator* (const AccurateFloat& a, const Integer& b)
   {
      return AccurateFloat(mpfr_mul_z, a.rep, b.get_rep(), MPFR_RNDN, isinf(b), Mul());
   }

   friend AccurateFloat operator* (const AccurateFloat& a, const Rational& b)
   {
      return AccurateFloat(mpfr_mul_q, a.rep, b.get_rep(), MPFR_RNDN, isinf(b), Mul());
   }

   friend AccurateFloat operator/ (const AccurateFloat& a, const AccurateFloat& b)
   {
      return AccurateFloat(mpfr_div, a.rep, b.rep, MPFR_RNDN);
   }

   friend AccurateFloat operator/ (const AccurateFloat& a, long b)
   {
      return AccurateFloat(mpfr_div_si, a.rep, b, MPFR_RNDN);
   }

   friend AccurateFloat operator/ (const AccurateFloat& a, double b)
   {
      return AccurateFloat(mpfr_div_d, a.rep, b, MPFR_RNDN);
   }

   friend AccurateFloat operator/ (const AccurateFloat& a, const Integer& b)
   {
      return AccurateFloat(mpfr_div_z, a.rep, b.get_rep(), MPFR_RNDN, isinf(b), Div());
   }

   friend AccurateFloat operator/ (const AccurateFloat& a, const Rational& b)
   {
      return AccurateFloat(mpfr_div_q, a.rep, b.get_rep(), MPFR_RNDN, isinf(b), Div());
   }

   friend AccurateFloat operator/ (long a, const AccurateFloat& b)
   {
      return AccurateFloat(mpfr_si_div, a, b.rep, MPFR_RNDN);
   }

   friend AccurateFloat operator/ (double a, const AccurateFloat& b)
   {
      return AccurateFloat(mpfr_d_div, a, b.rep, MPFR_RNDN);
   }

   friend AccurateFloat operator/ (const Integer& a, const AccurateFloat& b)
   {
      return (1L/b)*=a;
   }

   friend AccurateFloat operator/ (const Rational& a, const AccurateFloat& b)
   {
      return (1L/b)*a;
   }

   friend AccurateFloat operator<< (const AccurateFloat& a, unsigned long k)
   {
      return AccurateFloat(mpfr_mul_2ui, a.rep, k, MPFR_RNDZ);
   }

   friend AccurateFloat operator>> (const AccurateFloat& a, unsigned long k)
   {
      return AccurateFloat(mpfr_div_2ui, a.rep, k, MPFR_RNDZ);
   }

   friend AccurateFloat floor(const AccurateFloat& a)
   {
      return AccurateFloat(mpfr_floor, a.rep);
   }

   friend AccurateFloat ceil(const AccurateFloat& a)
   {
      return AccurateFloat(mpfr_ceil, a.rep);
   }

   friend AccurateFloat abs(const AccurateFloat& a)
   {
      return AccurateFloat(mpfr_abs, a.rep, MPFR_RNDN);
   }

   int compare(long b) const
   {
      return mpfr_cmp_si(rep, b);
   }

   int compare(int b) const { return compare(long(b)); }

   int compare(double b) const
   {
      return mpfr_cmp_d(rep, b);
   }

   int compare(const Integer& b) const
   {
      const int s1=isinf(*this), s2=isinf(b);
      if (__builtin_expect(s1 || s2, 0)) {
         return s1-s2;
      }
      return mpfr_cmp_z(rep, b.get_rep());
   }

   int compare(const Rational& b) const
   {
      const int s1=isinf(*this), s2=isinf(b);
      if (__builtin_expect(s1 || s2, 0)) {
         return s1-s2;
      }
      return mpfr_cmp_q(rep, b.get_rep());
   }

   int compare(const AccurateFloat& b) const
   {
      return mpfr_cmp(rep, b.rep);
   }

   typedef list comparable_with(int, long, double, Integer, Rational);

   bool operator! () const
   {
      return mpfr_zero_p(rep);
   }

   friend bool operator== (const AccurateFloat& a, const AccurateFloat& b)
   {
      return mpfr_equal_p(a.rep, b.rep);
   }

   friend bool operator< (const AccurateFloat& a, const AccurateFloat& b)
   {
      return mpfr_less_p(a.rep, b.rep);
   }

   friend bool operator<= (const AccurateFloat& a, const AccurateFloat& b)
   {
      return mpfr_lessequal_p(a.rep, b.rep);
   }

   friend bool abs_equal(const AccurateFloat& a, const AccurateFloat& b)
   {
      return !mpfr_cmpabs(a.rep, b.rep);
   }

   static AccurateFloat pow(const AccurateFloat& a, const AccurateFloat& b)
   {
      return AccurateFloat(mpfr_pow, a.rep, b.rep, MPFR_RNDN);
   }

   static AccurateFloat pow(const AccurateFloat& a, long k)
   {
      return AccurateFloat(mpfr_pow_si, a.rep, k, MPFR_RNDN);
   }

   static AccurateFloat pow(unsigned long a, unsigned long k)
   {
      return AccurateFloat(mpfr_ui_pow_ui, a, k, MPFR_RNDZ);
   }

   static AccurateFloat pow(unsigned long a, const AccurateFloat& b)
   {
      return AccurateFloat(mpfr_ui_pow, a, b.rep, MPFR_RNDN);
   }

   friend AccurateFloat sqrt(const AccurateFloat& a)
   {
      return AccurateFloat(mpfr_sqrt, a.rep, MPFR_RNDN);
   }

   // basic trigonometric functions
   friend AccurateFloat sin(const AccurateFloat& a)
   {
      return AccurateFloat(mpfr_sin, a.rep, MPFR_RNDN);
   }

   friend AccurateFloat cos(const AccurateFloat& a)
   {
      return AccurateFloat(mpfr_cos, a.rep, MPFR_RNDN);
   }

   friend AccurateFloat tan(const AccurateFloat& a)
   {
      return AccurateFloat(mpfr_tan, a.rep, MPFR_RNDN);
   }

   friend void sin_cos(AccurateFloat& s, AccurateFloat& c, const AccurateFloat& a)
   {
      mpfr_sin_cos(s.rep, c.rep, a.rep, MPFR_RNDN);
   }

   // basic arc functions
   friend AccurateFloat asin(const AccurateFloat& a)
   {
      return AccurateFloat(mpfr_asin, a.rep, MPFR_RNDN);
   }

   friend AccurateFloat acos(const AccurateFloat& a)
   {
      return AccurateFloat(mpfr_acos, a.rep, MPFR_RNDN);
   }

   friend AccurateFloat atan(const AccurateFloat& a)
   {
      return AccurateFloat(mpfr_atan, a.rep, MPFR_RNDN);
   }

   // basic hyperbolic trigonometric functions
   friend AccurateFloat sinh(const AccurateFloat& a)
   {
      return AccurateFloat(mpfr_sinh, a.rep, MPFR_RNDN);
   }

   friend AccurateFloat cosh(const AccurateFloat& a)
   {
      return AccurateFloat(mpfr_cosh, a.rep, MPFR_RNDN);
   }

   friend AccurateFloat tanh(const AccurateFloat& a)
   {
      return AccurateFloat(mpfr_tanh, a.rep, MPFR_RNDN);
   }

   friend void sinh_cosh(AccurateFloat& s, AccurateFloat& c, const AccurateFloat& a)
   {
      mpfr_sinh_cosh(s.rep, c.rep, a.rep, MPFR_RNDN);
   }

   // basic hyperbolic arc functions
   friend AccurateFloat asinh(const AccurateFloat& a)
   {
      return AccurateFloat(mpfr_asinh, a.rep, MPFR_RNDN);
   }

   friend AccurateFloat acosh(const AccurateFloat& a)
   {
      return AccurateFloat(mpfr_acosh, a.rep, MPFR_RNDN);
   }

   friend AccurateFloat atanh(const AccurateFloat& a)
   {
      return AccurateFloat(mpfr_atanh, a.rep, MPFR_RNDN);
   }


   friend AccurateFloat log(const AccurateFloat& a)
   {
      return AccurateFloat(mpfr_log, a.rep, MPFR_RNDN);
   }

   static AccurateFloat pi() { return AccurateFloat(mpfr_const_pi, MPFR_RNDN); }

   void putstr(std::ostream& os, std::ios::fmtflags) const;

   std::string to_string(int base=10) const
   {
      std::ostringstream os;
      putstr(os, std::ios::dec);
      return os.str();
   }

   friend
   std::ostream& operator<< (std::ostream& os, const AccurateFloat& a)
   {
      a.putstr(os, os.flags());
      return os;
   }
#if POLYMAKE_DEBUG
   void dump() const { std::cerr << *this << std::flush; }
#endif
};

}
namespace std {

inline void swap(pm::AccurateFloat& a, pm::AccurateFloat& b) { a.swap(b); }

template <>
class numeric_limits<pm::AccurateFloat> : public numeric_limits<pm::Rational> {
public:
   static const bool is_exact = false;
   static pm::AccurateFloat min() throw() { return pm::AccurateFloat(pm::maximal<pm::AccurateFloat>(),-1); }
   static pm::AccurateFloat infinity() throw() { return pm::AccurateFloat(pm::maximal<pm::AccurateFloat>()); }
   static pm::AccurateFloat max() throw() { return pm::AccurateFloat(pm::maximal<pm::AccurateFloat>()); }
};

}
namespace pm {

inline AccurateFloat operator+ (const AccurateFloat& a) { return a; }
inline AccurateFloat operator+ (const AccurateFloat& a, int b) { return a+long(b); }
inline AccurateFloat operator+ (long a, const AccurateFloat& b) { return b+a; }
inline AccurateFloat operator+ (int a, const AccurateFloat& b) { return b+long(a); }
inline AccurateFloat operator+ (double a, const AccurateFloat& b) { return b+a; }
inline AccurateFloat operator+ (const Integer& a, const AccurateFloat& b) { return b+a; }
inline AccurateFloat operator+ (const Rational& a, const AccurateFloat& b) { return b+a; }
inline AccurateFloat operator- (const AccurateFloat& a, int b) { return a-long(b); }
inline AccurateFloat operator- (int a, const AccurateFloat& b) { return long(a)-b; }
inline AccurateFloat operator* (const AccurateFloat& a, int b) { return a*long(b); }
inline AccurateFloat operator* (long a, const AccurateFloat& b) { return b*a; }
inline AccurateFloat operator* (int a, const AccurateFloat& b) { return b*long(a); }
inline AccurateFloat operator* (double a, const AccurateFloat& b) { return b+a; }
inline AccurateFloat operator* (const Integer& a, const AccurateFloat& b) { return b*a; }
inline AccurateFloat operator* (const Rational& a, const AccurateFloat& b) { return b*a; }
inline AccurateFloat operator/ (const AccurateFloat& a, int b) { return a/long(b); }
inline AccurateFloat operator/ (int a, const AccurateFloat& b) { return long(a)/b; }

inline bool operator!= (const AccurateFloat& a, const AccurateFloat& b) { return !(a==b); }
inline bool operator> (const AccurateFloat& a, const AccurateFloat& b) { return b<a; }
inline bool operator>= (const AccurateFloat& a, const AccurateFloat& b) { return b<=a; }

inline bool operator== (const AccurateFloat& a, int b) { return a.compare(long(b))==0; }
inline bool operator== (const AccurateFloat& a, long b) { return a.compare(b)==0; }
inline bool operator== (const AccurateFloat& a, double b) { return a.compare(b)==0; }
inline bool operator== (const AccurateFloat& a, const Integer& b) { return a.compare(b)==0; }
inline bool operator== (const AccurateFloat& a, const Rational& b) { return a.compare(b)==0; }
inline bool operator== (int a, const AccurateFloat& b) { return b.compare(long(a))==0; }
inline bool operator== (long a, const AccurateFloat& b) { return b.compare(a)==0; }
inline bool operator== (double a, const AccurateFloat& b) { return b.compare(a)==0; }
inline bool operator== (const Integer& a, const AccurateFloat& b) { return b.compare(a)==0; }
inline bool operator== (const Rational& a, const AccurateFloat& b) { return b.compare(a)==0; }

inline bool operator!= (const AccurateFloat& a, int b) { return a.compare(long(b))!=0; }
inline bool operator!= (const AccurateFloat& a, long b) { return a.compare(b)!=0; }
inline bool operator!= (const AccurateFloat& a, double b) { return a.compare(b)!=0; }
inline bool operator!= (const AccurateFloat& a, const Integer& b) { return a.compare(b)!=0; }
inline bool operator!= (const AccurateFloat& a, const Rational& b) { return a.compare(b)!=0; }
inline bool operator!= (int a, const AccurateFloat& b) { return b.compare(long(a))!=0; }
inline bool operator!= (long a, const AccurateFloat& b) { return b.compare(a)!=0; }
inline bool operator!= (double a, const AccurateFloat& b) { return b.compare(a)!=0; }
inline bool operator!= (const Integer& a, const AccurateFloat& b) { return b.compare(a)!=0; }
inline bool operator!= (const Rational& a, const AccurateFloat& b) { return b.compare(a)!=0; }

inline bool operator<= (const AccurateFloat& a, int b) { return a.compare(long(b))<=0; }
inline bool operator<= (const AccurateFloat& a, long b) { return a.compare(b)<=0; }
inline bool operator<= (const AccurateFloat& a, double b) { return a.compare(b)<=0; }
inline bool operator<= (const AccurateFloat& a, const Integer& b) { return a.compare(b)<=0; }
inline bool operator<= (const AccurateFloat& a, const Rational& b) { return a.compare(b)<=0; }
inline bool operator<= (int a, const AccurateFloat& b) { return b.compare(long(a))>=0; }
inline bool operator<= (long a, const AccurateFloat& b) { return b.compare(a)>=0; }
inline bool operator<= (double a, const AccurateFloat& b) { return b.compare(a)>=0; }
inline bool operator<= (const Integer& a, const AccurateFloat& b) { return b.compare(a)>=0; }
inline bool operator<= (const Rational& a, const AccurateFloat& b) { return b.compare(a)>=0; }

inline bool operator>= (const AccurateFloat& a, int b) { return a.compare(long(b))>=0; }
inline bool operator>= (const AccurateFloat& a, long b) { return a.compare(b)>=0; }
inline bool operator>= (const AccurateFloat& a, double b) { return a.compare(b)>=0; }
inline bool operator>= (const AccurateFloat& a, const Integer& b) { return a.compare(b)>=0; }
inline bool operator>= (const AccurateFloat& a, const Rational& b) { return a.compare(b)>=0; }
inline bool operator>= (int a, const AccurateFloat& b) { return b.compare(long(a))<=0; }
inline bool operator>= (long a, const AccurateFloat& b) { return b.compare(a)<=0; }
inline bool operator>= (double a, const AccurateFloat& b) { return b.compare(a)<=0; }
inline bool operator>= (const Integer& a, const AccurateFloat& b) { return b.compare(a)<=0; }
inline bool operator>= (const Rational& a, const AccurateFloat& b) { return b.compare(a)<=0; }

inline bool operator< (const AccurateFloat& a, int b) { return a.compare(long(b))<0; }
inline bool operator< (const AccurateFloat& a, long b) { return a.compare(b)<0; }
inline bool operator< (const AccurateFloat& a, double b) { return a.compare(b)<0; }
inline bool operator< (const AccurateFloat& a, const Integer& b) { return a.compare(b)<0; }
inline bool operator< (const AccurateFloat& a, const Rational& b) { return a.compare(b)<0; }
inline bool operator< (int a, const AccurateFloat& b) { return b.compare(long(a))>0; }
inline bool operator< (long a, const AccurateFloat& b) { return b.compare(a)>0; }
inline bool operator< (double a, const AccurateFloat& b) { return b.compare(a)>0; }
inline bool operator< (const Integer& a, const AccurateFloat& b) { return b.compare(a)>0; }
inline bool operator< (const Rational& a, const AccurateFloat& b) { return b.compare(a)>0; }

inline bool operator> (const AccurateFloat& a, int b) { return a.compare(long(b))>0; }
inline bool operator> (const AccurateFloat& a, long b) { return a.compare(b)>0; }
inline bool operator> (const AccurateFloat& a, double b) { return a.compare(b)>0; }
inline bool operator> (const AccurateFloat& a, const Integer& b) { return a.compare(b)>0; }
inline bool operator> (const AccurateFloat& a, const Rational& b) { return a.compare(b)>0; }
inline bool operator> (int a, const AccurateFloat& b) { return b.compare(long(a))<0; }
inline bool operator> (long a, const AccurateFloat& b) { return b.compare(a)<0; }
inline bool operator> (double a, const AccurateFloat& b) { return b.compare(a)<0; }
inline bool operator> (const Integer& a, const AccurateFloat& b) { return b.compare(a)<0; }
inline bool operator> (const Rational& a, const AccurateFloat& b) { return b.compare(a)<0; }

template <> inline
cmp_value sign(const AccurateFloat& a) { return sign(mpfr_sgn(a.get_rep())); }

template <>
class conv<AccurateFloat, Integer> : public conv_by_cast<AccurateFloat, Integer> {};

template <>
class conv<Integer, AccurateFloat> : public conv_by_cast<Integer, AccurateFloat> {};

template <>
class conv<AccurateFloat, Rational> : public conv_by_cast<AccurateFloat, Rational> {};

template <>
class conv<Rational, AccurateFloat> : public conv_by_cast<Rational, AccurateFloat> {};

template <>
class conv<double, AccurateFloat> : public conv_by_cast<double, AccurateFloat> {};

template <>
class conv<AccurateFloat, double> {
public:
   typedef AccurateFloat argument_type;
   typedef double result_type;
   result_type operator() (const AccurateFloat& a) const { return mpfr_get_d(a.get_rep(), MPFR_RNDN); }
};

template <>
struct algebraic_traits<AccurateFloat> {
   typedef AccurateFloat ring_type;
   typedef AccurateFloat field_type;
};

namespace operations {

template <>
struct cmp_scalar<AccurateFloat, AccurateFloat, true> : cmp_GMP_based<AccurateFloat> {};

} }
namespace polymake {
using pm::AccurateFloat;
}

#endif // POLYMAKE_ACCURATERATIONAL_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
