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

/** @file Rational.h
    @brief Implementation of pm::Rational class
*/


#ifndef POLYMAKE_RATIONAL_H
#define POLYMAKE_RATIONAL_H

#include "polymake/Integer.h"

namespace pm {
namespace GMP {

class TempRational : public MP_RAT {
protected:
   /// never instantiate this class: it is a pure masquerade
   TempRational();
   ~TempRational();
};

enum proxy_kind { num, den };

template <proxy_kind kind, bool _canonicalize=true>
class Proxy : public Integer {
private:
   void canonicalize();

   /// both undefined
   Proxy(const Proxy&);
   Proxy();

   friend class pm::Rational;
public:

   template <typename T>
   Proxy& operator= (const T& b)
   {
      Integer::operator=(b);
      canonicalize();
      return *this;
   }

   friend
   std::istream& operator>> (std::istream& in, Proxy& me)
   {
      in >> static_cast<Integer&>(me);
      me.canonicalize();
      return in;
   }

   Proxy& operator++()
   {
      Integer::operator++();
      canonicalize();
      return *this;
   }

   Proxy& operator--()
   {
      Integer::operator--();
      canonicalize();
      return *this;
   }

   template <typename T>
   Proxy& operator+= (const T& b)
   {
      Integer::operator+=(b);
      canonicalize();
      return *this;
   }

   template <typename T>
   Proxy& operator-= (const T& b)
   {
      Integer::operator-=(b);
      canonicalize();
      return *this;
   }

   template <typename T>
   Proxy& operator*= (const T& b)
   {
      Integer::operator*=(b);
      canonicalize();
      return *this;
   }

   template <typename T>
   Proxy& operator/= (const T& b)
   {
      Integer::operator/=(b);
      canonicalize();
      return *this;
   }

   template <typename T>
   Proxy& operator%= (const T& b)
   {
      Integer::operator%=(b);
      canonicalize();
      return *this;
   }

   template <typename T>
   Proxy& operator<<= (const T& b)
   {
      Integer::operator<<=(b);
      canonicalize();
      return *this;
   }

   template <typename T>
   Proxy& operator>>= (const T& b)
   {
      Integer::operator>>=(b);
      canonicalize();
      return *this;
   }

#if defined(__GNUC__)
#if __GNUC__ == 4 && __GNUC_MINOR < 3
   friend int isfinite(const Proxy& me) { return isfinite(static_cast<const Integer&>(me)); }
#endif
#endif
};
}

inline int isfinite(const Rational& a);
inline int isinf(const Rational& a);

/// @brief A class for rational numbers.
class Rational {
   friend class Integer;
   template <typename> friend class std::numeric_limits;
   friend struct spec_object_traits<Rational>;
   template <GMP::proxy_kind, bool> friend class GMP::Proxy;
protected:
   /// GMP's representation
   mpq_t rep;

   static void _init_set_inf(mpq_ptr rep, int s)
   {
      Integer::_init_set_inf(mpq_numref(rep), s);
      mpz_init_set_ui(mpq_denref(rep), 1);
   }
   static void _init_set_inf(mpq_ptr rep, mpz_srcptr b)
   {
      Integer::_init_set_inf(mpq_numref(rep), b);
      mpz_init_set_ui(mpq_denref(rep), 1);
   }
   static void _init_set_inf(mpq_ptr rep, mpz_srcptr b, int inv)
   {
      Integer::_init_set_inf(mpq_numref(rep), b, inv);
      mpz_init_set_ui(mpq_denref(rep), 1);
   }
   static void _init_set_inf(mpq_ptr rep, mpq_srcptr b)
   {
      Integer::_init_set_inf(mpq_numref(rep), mpq_numref(b));
      mpz_init_set_ui(mpq_denref(rep), 1);
   }
   static void _init_set_inf(mpq_ptr rep, mpq_srcptr b, int inv)
   {
      Integer::_init_set_inf(mpq_numref(rep), mpq_numref(b), inv);
      mpz_init_set_ui(mpq_denref(rep), 1);
   }
   static void _set_inf(mpq_ptr rep, int s)
   {
      Integer::_set_inf(mpq_numref(rep), s);
      mpz_set_ui(mpq_denref(rep), 1);
   }
   static void _set_inf(mpq_ptr rep, mpz_srcptr b)
   {
      Integer::_set_inf(mpq_numref(rep), b);
      mpz_set_ui(mpq_denref(rep), 1);
   }
   static void _set_inf(mpq_ptr rep, mpz_srcptr b, int inv)
   {
      Integer::_set_inf(mpq_numref(rep), b, inv);
      mpz_set_ui(mpq_denref(rep), 1);
   }
   static void _set_inf(mpq_ptr rep, mpq_srcptr b)
   {
      Integer::_set_inf(mpq_numref(rep), mpq_numref(b));
      mpz_set_ui(mpq_denref(rep), 1);
   }
   static void _set_inf(mpq_ptr rep, mpq_srcptr b, int inv)
   {
      Integer::_set_inf(mpq_numref(rep), mpq_numref(b), inv);
      mpz_set_ui(mpq_denref(rep), 1);
   }
   static void _inf_inv_sign(mpq_ptr rep, long s, bool division=false)
   {
      Integer::_inf_inv_sign(mpq_numref(rep), s, division);
   }

   void canonicalize()
   {
      if (__builtin_expect(mpz_sgn(mpq_denref(rep)),1))
         mpq_canonicalize(rep);
      else if (mpz_sgn(mpq_numref(rep)))
         throw GMP::ZeroDivide();
      else
         throw GMP::NaN();
   }
public:

#if defined(__GMP_PLUSPLUS__)
   //constructs from gmp's mpz_class as numerator and denominator
   Rational(const mpz_class& num, const mpz_class& den)
   {
      mpz_init_set(mpq_numref(rep), num.get_mpz_t());
      mpz_init_set(mpq_denref(rep), den.get_mpz_t());
      canonicalize();
   }
#endif

   /// Initializes to 0.
   Rational() { mpq_init(rep); }

   Rational(const Rational& a)
   {
      if (__builtin_expect(isfinite(a),1)) {
         mpz_init_set(mpq_numref(rep), mpq_numref(a.rep));
         mpz_init_set(mpq_denref(rep), mpq_denref(a.rep));
      } else {
         _init_set_inf(rep, a.rep);
      }
   }

   Rational(const Integer& a)
   {
      if (__builtin_expect(isfinite(a),1)) {
         mpz_init_set(mpq_numref(rep), a.rep);
         mpz_init_set_ui(mpq_denref(rep), 1);
      } else {
         _init_set_inf(rep, a.rep);
      }
   }

   /// Create a Rational from a long.
   Rational(long num)
   {
      mpz_init_set_si(mpq_numref(rep), num);
      mpz_init_set_ui(mpq_denref(rep), 1);
   }

   /// Create a Rational from an int.
   Rational(int num)
   {
      mpz_init_set_si(mpq_numref(rep), num);
      mpz_init_set_ui(mpq_denref(rep), 1);
   }

   /// Create a Rational from an AccurateFloat
   explicit Rational(const AccurateFloat&);

   /// Create a Rational as the quotient of two Integers.
   Rational(const Integer& num, const Integer& den)
   {
      if (__builtin_expect(isfinite(num),1)) {
         if (__builtin_expect(isfinite(den),1)) {
            mpz_init_set(mpq_numref(rep), num.rep);
            mpz_init_set(mpq_denref(rep), den.rep);
            canonicalize();
         } else {
            mpz_init_set_ui(mpq_numref(rep), 0);
            mpz_init_set_ui(mpq_denref(rep), 1);
         }
      } else if (isfinite(den)) {
         _init_set_inf(rep, den.rep, mpz_sgn(num.rep));
      } else {
         throw GMP::NaN();
      }
   }

   /// Create a Rational as the quotient of two longs.
   Rational(long num, long den)
   {
      mpz_init_set_si(mpq_numref(rep), num);
      mpz_init_set_si(mpq_denref(rep), den);
      canonicalize();
   }

   /// Create a Rational as the quotient of an Integer @a num and a long @den.
   Rational(const Integer& num, long den)
   {
      if (__builtin_expect(isfinite(num),1)) {
         mpz_init_set(mpq_numref(rep), num.rep);
         mpz_init_set_si(mpq_denref(rep), den);
         canonicalize();
      } else {
         _init_set_inf(rep, num.rep, den<0 ? -1 : 1);
      }
   }

   /// Create a Rational as the quotient of a long @a num and an Integer @den.
   Rational(long num, const Integer& den)
   {
      if (__builtin_expect(isfinite(den),1)) {
         mpz_init_set_si(mpq_numref(rep), num);
         mpz_init_set(mpq_denref(rep), den.rep);
         canonicalize();
      } else {
         mpz_init_set_ui(mpq_numref(rep), 0);
         mpz_init_set_ui(mpq_denref(rep), 1);
      }
   }

   /// Create a Rational from a double.
   Rational(double b)
   {
      const int i=isinf(b);
      if (__builtin_expect(i,0)) {
         _init_set_inf(rep, i);
      } else {
         mpq_init(rep);
         mpq_set_d(rep,b);
      }
   }

   explicit Rational(const char* s)
   {
      mpq_init(rep);
      try {
         _set(s);
      }
      catch (const GMP::error&) {
         mpq_clear(rep);
         throw;
      }
   }

   explicit Rational(mpq_srcptr src)
   {
      mpz_init_set(mpq_numref(rep), mpq_numref(src));
      mpz_init_set(mpq_denref(rep), mpq_denref(src));
      canonicalize();
   }

   explicit Rational(mpz_srcptr num_src)
   {
      mpz_init_set(mpq_numref(rep), num_src);
      mpz_init_set_ui(mpq_denref(rep), 1);
   }

   Rational(mpz_srcptr num_src, mpz_srcptr den_src)
   {
      mpz_init_set(mpq_numref(rep), num_src);
      mpz_init_set(mpq_denref(rep), den_src);
      canonicalize();
   }

   explicit Rational(GMP::TempRational& tmp)
   {
      rep[0]=tmp;
      canonicalize();
   }

   explicit Rational(GMP::TempInteger& tmp_num)
   {
      *mpq_numref(rep)=tmp_num;
      mpz_init_set_ui(mpq_denref(rep), 1);
   }

   Rational(GMP::TempInteger& tmp_num, GMP::TempInteger& tmp_den)
   {
      *mpq_numref(rep)=tmp_num;
      *mpq_denref(rep)=tmp_den;
      canonicalize();
   }

protected:
   template <typename T>
   explicit Rational(maximal<T>, int s=1)
   {
      _init_set_inf(rep,s);
   }
   template <typename T>
   explicit Rational(maximal<T>, mpz_srcptr b)
   {
      _init_set_inf(rep,b);
   }
   template <typename T>
   explicit Rational(maximal<T>, mpz_srcptr b, int inv)
   {
      _init_set_inf(rep,b,inv);
   }
   template <typename T>
   explicit Rational(maximal<T>, mpq_srcptr b)
   {
      _init_set_inf(rep,b);
   }
   template <typename T>
   explicit Rational(maximal<T>, mpq_srcptr b, int inv)
   {
      _init_set_inf(rep,b,inv);
   }

   void canonicalize_sign()
   {
      if (mpz_sgn(mpq_denref(rep))<0) {
         mpz_neg(mpq_numref(rep), mpq_numref(rep));
         mpz_neg(mpq_denref(rep), mpq_denref(rep));
      }
   }

public:
   Rational(void (*f)(mpq_ptr,mpq_srcptr), mpq_srcptr a)
   {
      mpq_init(rep);
      f(rep,a);
   }

   template <typename Arg>
   Rational(void (*f)(mpq_ptr,mpq_srcptr,Arg), mpq_srcptr a, Arg b)
   {
      mpq_init(rep);
      f(rep,a,b);
   }

   template <typename Arg>
   Rational(void (*numf)(mpz_ptr,Arg), Arg a, mpz_srcptr den)
   {
      mpz_init(mpq_numref(rep));
      numf(mpq_numref(rep),a);
      mpz_init_set(mpq_denref(rep),den);
      canonicalize_sign();
   }

   template <typename Arg>
   Rational(mpz_srcptr num, void (*denf)(mpz_ptr,Arg), Arg a)
   {
      mpz_init_set(mpq_numref(rep),num);
      mpz_init(mpq_denref(rep));
      denf(mpq_denref(rep),a);
      canonicalize_sign();
   }

   template <typename Arg1, typename Arg2, bool _canonicalize>
   Rational(void (*numf)(mpz_ptr,Arg1,Arg2), Arg1 a, Arg2 b, mpz_srcptr den, bool2type<_canonicalize>)
   {
      mpz_init(mpq_numref(rep));
      numf(mpq_numref(rep),a,b);
      mpz_init_set(mpq_denref(rep),den);
      if (_canonicalize) canonicalize_sign();
   }

   template <typename Arg1, typename Arg2, bool _canonicalize>
   Rational(mpz_srcptr num, void (*denf)(mpz_ptr,Arg1,Arg2), Arg1 a, Arg2 b, bool2type<_canonicalize>)
   {
      mpz_init_set(mpq_numref(rep),num);
      mpz_init(mpq_denref(rep));
      denf(mpq_denref(rep),a,b);
      if (_canonicalize) canonicalize_sign();
   }

   template <typename Arg1, typename Arg2>
   Rational(void (*numf)(mpz_ptr,Arg1,Arg2), mpz_srcptr num, Arg1 a, Arg2 b, mpz_srcptr den)
   {
      mpz_init_set(mpq_numref(rep),num);
      numf(mpq_numref(rep),a,b);
      mpz_init_set(mpq_denref(rep),den);
   }

   template <typename Arg1, typename Arg2>
   Rational(unsigned long num, void (*denf)(mpz_ptr,Arg1,Arg2), Arg1 a, Arg2 b)
   {
      mpz_init_set_ui(mpq_numref(rep),num);
      mpz_init(mpq_denref(rep));
      denf(mpq_denref(rep),a,b);
   }

   template <typename Arg1, typename Arg2>
   Rational(void (*numf)(mpz_ptr,Arg1,Arg2), Arg1 a, Arg2 b, unsigned long den)
   {
      mpz_init(mpq_numref(rep));
      numf(mpq_numref(rep),a,b);
      mpz_init_set_ui(mpq_denref(rep),den);
   }

   template <typename Arg1, typename Arg2>
   Rational(mpz_srcptr num, void (*denf)(mpz_ptr,Arg1,Arg2), Arg1 a, Arg2 b, mpz_srcptr den)
   {
      mpz_init_set(mpq_numref(rep),num);
      mpz_init_set(mpq_denref(rep),den);
      denf(mpq_denref(rep),a,b);
   }

   template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, bool _canonicalize>
   Rational(void (*numf)(mpz_ptr,Arg1,Arg2), Arg1 a, Arg2 b,
            void (*denf)(mpz_ptr,Arg3,Arg4), Arg3 c, Arg4 d,
            bool2type<_canonicalize>)
   {
      mpq_init(rep);
      numf(mpq_numref(rep),a,b);
      denf(mpq_denref(rep),c,d);
      if (_canonicalize) canonicalize_sign();
   }

   Rational(gmp_randstate_t rnd, unsigned long bits)
   {
      mpq_init(rep);
      mpz_urandomb(mpq_numref(rep), rnd, bits);
      mpq_div_2exp(rep, rep, bits);
   }

   ~Rational()
   {
      mpq_clear(rep);
#if POLYMAKE_DEBUG
      POLYMAKE_DEBUG_METHOD(Rational,dump);
#endif
   }

   typedef GMP::Proxy<GMP::num> numerator_type;
   typedef GMP::Proxy<GMP::den> denominator_type;

   friend
   const numerator_type& numerator(const Rational& a)
   {
      return *reinterpret_cast<const GMP::Proxy<GMP::num>*>(mpq_numref(a.rep));
   }

   friend
   numerator_type& numerator(Rational& a)
   {
      return *reinterpret_cast<GMP::Proxy<GMP::num>*>(mpq_numref(a.rep));
   }

   friend
   const denominator_type& denominator(const Rational& a)
   {
      return *reinterpret_cast<const GMP::Proxy<GMP::den>*>(mpq_denref(a.rep));
   }

   friend
   denominator_type& denominator(Rational& a)
   {
      return *reinterpret_cast<GMP::Proxy<GMP::den>*>(mpq_denref(a.rep));
   }

   friend GMP::Proxy<GMP::num,false>& numerator_nocanon(Rational& a)
   {
      return *reinterpret_cast<GMP::Proxy<GMP::num,false>*>(mpq_numref(a.rep));
   }

   Rational& set(const Integer& num, const Integer& den)
   {
      const bool fn=isfinite(num), fd=isfinite(den);
      if (__builtin_expect(fn && fd, 1))
         set(num.rep, den.rep);
      else if (fn)
         *this=0;
      else if (fd)
         _set_inf(rep, num.rep, mpz_sgn(den.rep));
      else
         throw GMP::NaN();
      return *this;
   }

   Rational& set(long num, long den)
   {
      mpz_set_si(mpq_numref(rep),num);
      mpz_set_si(mpq_denref(rep),den);
      canonicalize();
      return *this;
   }

protected:
   void _set(const char *s);
public:

   /** Conversion from a printable representation.
       Numerator and denominator are expected delimited by `/'.
       Omitted denominator assumed equal to 1.
   */
   Rational& set(const char *s)
   {
      if (__builtin_expect(!isfinite(*this),0))
         mpz_init(mpq_numref(rep));
      _set(s);
      return *this;
   }

   Rational& operator= (const Rational& b)
   {
      const bool f1=isfinite(*this), f2=isfinite(b);
      if (__builtin_expect(f1 && f2, 1))
         mpq_set(rep, b.rep);
      else if (f2) {
         mpz_init_set(mpq_numref(rep), mpq_numref(b.rep));
         mpz_set(mpq_denref(rep), mpq_denref(b.rep));
      } else
         _set_inf(rep, b.rep);
      return *this;
   }

   Rational& set(mpz_srcptr num_src, mpz_srcptr den_src)
   {
      mpq_set_num(rep, num_src);
      mpq_set_den(rep, den_src);
      canonicalize();
      return *this;
   }

   /// for the seldom case of unwrapped GMP objects coexisting with us
   Rational& set(mpq_srcptr src)
   {
      mpq_set(rep, src);
      return *this;
   }

   Rational& operator= (const Integer& b)
   {
      const bool f1=isfinite(*this), f2=isfinite(b);
      if (__builtin_expect(f1 && f2, 1))
         mpq_set_z(rep, b.rep);
      else if (f2) {
         mpz_init_set(mpq_numref(rep), b.rep);
         mpz_set_ui(mpq_denref(rep), 1);
      } else
         _set_inf(rep, b.rep);
      return *this;
   }

   Rational& operator= (long b)
   {
      if (__builtin_expect(isfinite(*this),1))
         mpq_set_si(rep, b, 1);
      else {
         mpz_init_set_si(mpq_numref(rep), b);
         mpz_set_ui(mpq_denref(rep), 1);
      }
      return *this;
   }

   Rational& operator= (int b) { return operator=(long(b)); }

   Rational& operator= (double b)
   {
      const bool f1=isfinite(*this);
      const int i2=isinf(b);
      if (__builtin_expect(f1 && !i2, 1))
         mpq_set_d(rep, b);
      else if (!i2) {
         mpz_init_set_d(mpq_numref(rep), b);
         mpz_set_ui(mpq_denref(rep), 1);
      } else
         _set_inf(rep, i2);
      return *this;
   }

   Rational& operator= (const AccurateFloat&);

   double to_double() const
   {
      const int i=isinf(*this);
      if (__builtin_expect(i,0))
         return i*std::numeric_limits<double>::infinity();
      return mpq_get_d(rep);
   }

   long to_long() const
   {
      return static_cast<Integer>(*this).to_long();
   }

   int to_int() const
   {
      return static_cast<Integer>(*this).to_int();
   }

   /// Convert rational to string.
   std::string to_string(int base=10) const;

   bool non_zero() const
   {
      return mpq_sgn(rep);
   }

   mpq_srcptr get_rep() const { return rep; }

   /// Swaps the values.
   void swap(Rational& b) { mpq_swap(rep, b.rep); }

   /** Accelerated combination of copy constructor and destructor.
       Aimed to be used in container classes only! */
   friend void relocate(Rational* from, Rational* to)
   {
      to->rep[0] = from->rep[0];
   }

   Rational& operator++ ()
   {
      if (__builtin_expect(isfinite(*this),1)) mpz_add(mpq_numref(rep), mpq_numref(rep), mpq_denref(rep));
      return *this;
   }

   Rational& operator-- ()
   {
      if (__builtin_expect(isfinite(*this),1)) mpz_sub(mpq_numref(rep), mpq_numref(rep), mpq_denref(rep));
      return *this;
   }

   /// In-place negation.
   Rational& negate()
   {
      mpq_numref(rep)->_mp_size*=-1;
      return *this;
   }

   /// Addition %operator.
   Rational& operator+= (const Rational& b)
   {
      const bool f1=isfinite(*this), f2=isfinite(b);
      if (__builtin_expect(f1 && f2, 1))
         mpq_add(rep, rep, b.rep);
      else if (f1)
         _set_inf(rep, b.rep);
      else if (!f2 && isinf(*this)!=isinf(b))
         throw GMP::NaN();
      return *this;
   }

   /// Addition %operator.
   Rational& operator+= (const Integer& b)
   {
      const bool f1=isfinite(*this), f2=isfinite(b);
      if (__builtin_expect(f1 && f2, 1)) {
         mpz_addmul(mpq_numref(rep), mpq_denref(rep), b.rep);
      } else if (f1)
         _set_inf(rep, b.rep);
      else if (!f2 && isinf(*this)!=isinf(b))
         throw GMP::NaN();
      return *this;
   }

   /// Addition %operator.
   Rational& operator+= (long b)
   {
      if (__builtin_expect(isfinite(*this),1)) {
         if (b>=0)
            mpz_addmul_ui(mpq_numref(rep),  mpq_denref(rep), b);
         else
            mpz_submul_ui(mpq_numref(rep),  mpq_denref(rep), -b);
      }
      return *this;
   }

   /// Subtraction %operator.
   Rational& operator-= (const Rational& b)
   {
      const bool f1=isfinite(*this), f2=isfinite(b);
      if (__builtin_expect(f1 && f2, 1))
         mpq_sub(rep, rep, b.rep);
      else if (f1)
         _set_inf(rep, b.rep, -1);
      else if (isinf(*this)==isinf(b))
         throw GMP::NaN();
      return *this;
   }

   /// Subtraction %operator.
   Rational& operator-= (const Integer& b)
   {
      const bool f1=isfinite(*this), f2=isfinite(b);
      if (__builtin_expect(f1 && f2, 1))
         mpz_submul(mpq_numref(rep), mpq_denref(rep), b.rep);
      else if (f1)
         _set_inf(rep, b.rep, -1);
      else if (isinf(*this)==isinf(b))
         throw GMP::NaN();
      return *this;
   }

   /// Subtraction %operator.
   Rational& operator-= (long b)
   {
      if (__builtin_expect(isfinite(*this),1)) {
         if (b>=0)
            mpz_submul_ui(mpq_numref(rep), mpq_denref(rep), b);
         else
            mpz_addmul_ui(mpq_numref(rep), mpq_denref(rep), -b);
      }
      return *this;
   }

   /// Multiplication %operator.
   Rational& operator*= (const Rational& b)
   {
      const bool f1=isfinite(*this), f2=isfinite(b);
      if (__builtin_expect(f1 && f2, 1))
         mpq_mul(rep, rep, b.rep);
      else
         _inf_inv_sign(rep, mpq_sgn(b.rep));
      return *this;
   }

   /// Multiplication %operator.
   inline Rational& operator*= (const Integer& b);

   /// Multiplication %operator.
   Rational& operator*= (long b)
   {
      if (__builtin_expect(isfinite(*this),1)) {
         if (!non_zero()) return *this;
         if (b) {
            unsigned long g=mpz_gcd_ui(0, mpq_denref(rep), b>=0 ? b : -b);
            if (g==1) {
               mpz_mul_si(mpq_numref(rep), mpq_numref(rep), b);
            } else {
               mpz_mul_si(mpq_numref(rep), mpq_numref(rep), b/(long)g);
               mpz_divexact_ui(mpq_denref(rep), mpq_denref(rep), g);
            }
         } else {
            *this=0;
         }
      } else
         _inf_inv_sign(rep,b);
      return *this;
   }

   /// Division %operator.
   Rational& operator/= (const Rational& b)
   {
      const bool f1=isfinite(*this), f2=isfinite(b);
      if (__builtin_expect(f1 && f2, 1)) {
         if (__builtin_expect(!b.non_zero(), 0))
            throw GMP::ZeroDivide();
         else
            mpq_div(rep, rep, b.rep);
      } else if (f1)
         *this=0;
      else if (f2)
         _inf_inv_sign(rep, mpq_sgn(b.rep), true);
      else
         throw GMP::NaN();
      return *this;
   }

   /// Division %operator.
   inline Rational& operator/= (const Integer& b);

   /// Division %operator.
   Rational& operator/= (long b)
   {
      if (__builtin_expect(isfinite(*this),1)) {
         if (__builtin_expect(!b,0))
            throw GMP::ZeroDivide();
         else if (__builtin_expect(mpq_sgn(rep),1)) {
            const unsigned long babs= b>0 ? b : -b;
            unsigned long g=mpz_gcd_ui(0, mpq_numref(rep), babs);
            if (g==1) {
               mpz_mul_ui(mpq_denref(rep), mpq_denref(rep), babs);
            } else {
               mpz_mul_ui(mpq_denref(rep), mpq_denref(rep), babs/g);
               mpz_divexact_ui(mpq_numref(rep), mpq_numref(rep), g);
            }
            if (b<0) mpz_neg(mpq_numref(rep), mpq_numref(rep));
         }
      } else
         _inf_inv_sign(rep,b,true);
      return *this;
   }

   /// Multiply with 2**k.
   Rational& operator<<= (unsigned long k)
   {
      if (__builtin_expect(isfinite(*this),1))
         mpq_mul_2exp(rep, rep, k);
      return *this;
   }
   
   /// Divide thru 2**k.
   Rational& operator>>= (unsigned long k)
   {
      if (__builtin_expect(isfinite(*this),1))
         mpq_div_2exp(rep, rep, k);
      return *this;
   }

   /// Addition of two Rationals.
   friend Rational operator+ (const Rational& a, const Rational& b)
   {
      const bool f1=isfinite(a), f2=isfinite(b);
      if (__builtin_expect(f1 && f2, 1))
         return Rational(mpq_add, a.rep, b.get_rep());
      if (f2) return a;
      if (!f1 && isinf(a)!=isinf(b))
         throw GMP::NaN();
      return b;
   }

   /// Addition
   friend Rational operator+ (const Rational& a, const Integer& b)
   {
      const bool f1=isfinite(a), f2=isfinite(b);
      if (__builtin_expect(f1 && f2, 1))
         return Rational(mpz_addmul, mpq_numref(a.rep), mpq_denref(a.rep), b.get_rep(), mpq_denref(a.rep));
      if (f2)
         return a;
      if (!f1 && isinf(a)!=isinf(b))
         throw GMP::NaN();
      return Rational(maximal<Rational>(), b.get_rep());
   }

   /// Addition
   friend Rational operator+ (const Rational& a, long b)
   {
      if (__builtin_expect(isfinite(a),1)) {
         if (b>=0)
            return Rational(mpz_addmul_ui, mpq_numref(a.rep), mpq_denref(a.rep), (unsigned long)b,
                            mpq_denref(a.rep));
         else
            return Rational(mpz_submul_ui, mpq_numref(a.rep), mpq_denref(a.rep), (unsigned long)(-b),
                            mpq_denref(a.rep));
      }
      return a;
   }

   /// Subtraction
   friend Rational operator- (const Rational& a, const Rational& b)
   {
      const bool f1=isfinite(a), f2=isfinite(b);
      if (__builtin_expect(f1 && f2, 1))
         return Rational(mpq_sub, a.rep, b.rep);
      if (f2) return a;
      if (isinf(a)==isinf(b))
         throw GMP::NaN();
      return Rational(maximal<Rational>(), b.rep, -1);
   }

   /// Subtraction
   friend Rational operator- (const Rational& a, const Integer& b)
   {
      const bool f1=isfinite(a), f2=isfinite(b);
      if (__builtin_expect(f1 && f2, 1))
         return Rational(mpz_submul, mpq_numref(a.rep), mpq_denref(a.rep), b.get_rep(), mpq_denref(a.rep));
      if (f2)
         return a;
      if (isinf(a)==isinf(b))
         throw GMP::NaN();
      return Rational(maximal<Rational>(), b.get_rep(), -1);
   }

   /// Subtraction
   friend Rational operator- (const Integer& a, const Rational& b)
   {
      const bool f1=isfinite(a), f2=isfinite(b);
      if (__builtin_expect(f1 && f2, 1)) {
         mpz_t minus_num;
         return Rational(mpz_addmul, Integer::_tmp_negate(minus_num,mpq_numref(b.rep)), mpq_denref(b.rep), a.get_rep(),
                         mpq_denref(b.rep));
      }
      if (f2) return Rational(maximal<Rational>(), a.get_rep());
      if (isinf(a)==isinf(b))
         throw GMP::NaN();
      return Rational(maximal<Rational>(), b.rep, -1);
   }

   /// Subtraction
   friend Rational operator- (const Rational& a, long b)
   {
      if (__builtin_expect(isfinite(a),1)) {
         if (b>=0)
            return Rational(mpz_submul_ui, mpq_numref(a.rep), mpq_denref(a.rep), (unsigned long)b,
                            mpq_denref(a.rep));
         else
            return Rational(mpz_addmul_ui, mpq_numref(a.rep), mpq_denref(a.rep), (unsigned long)(-b),
                            mpq_denref(a.rep));
      }
      return a;
   }

   /// Subtraction
   friend Rational operator- (long a, const Rational& b)
   {
      if (__builtin_expect(isfinite(b),1)) {
         mpz_t minus_num;
         Integer::_tmp_negate(minus_num,mpq_numref(b.rep));
         if (a>=0) {
            return Rational(mpz_addmul_ui, minus_num, mpq_denref(b.rep), (unsigned long)a, mpq_denref(b.rep));
         } else {
            return Rational(mpz_submul_ui, minus_num, mpq_denref(b.rep), (unsigned long)(-a), mpq_denref(b.rep));
         }
      }
      return Rational(maximal<Rational>(), b.rep, -1);
   }

   /// Subtraction
   friend Rational operator- (const Rational& a)
   {
      if (__builtin_expect(isfinite(a),1))
         return Rational(mpq_neg,a.rep);
      return Rational(maximal<Rational>(), a.rep, -1);
   }

   /// Multiplication
   friend Rational operator* (const Rational& a, const Rational& b)
   {
      const bool f1=isfinite(a), f2=isfinite(b);
      if (__builtin_expect(f1 && f2, 1))
         return Rational(mpq_mul,a.rep,b.rep);
      const int s=mpq_sgn(a.rep)*mpq_sgn(b.rep);
      if (!s) throw GMP::NaN();
      return Rational(maximal<Rational>(), s);
   }

   /// Multiplication
   friend Rational operator* (const Rational& a, const Integer& b);

   /// Multiplication
   friend Rational operator* (const Rational& a, long b)
   {
      if (__builtin_expect(isfinite(a),1)) {
         if (!b || !a.non_zero()) return Rational();
         unsigned long g=mpz_gcd_ui(0, mpq_denref(a.rep), b>=0 ? b : -b);
         if (g==1) {
            return Rational(mpz_mul_si, mpq_numref(a.rep), b, mpq_denref(a.rep), False());
         } else {
            return Rational(mpz_mul_si, mpq_numref(a.rep), b/(long)g,
                            mpz_divexact_ui, mpq_denref(a.rep), g, False());
         }
      }
      if (!b) throw GMP::NaN();
      return Rational(maximal<Rational>(), a.rep, sign(b));
   }

   /// Division
   friend Rational operator/ (const Rational& a, const Rational& b)
   {
      const bool f1=isfinite(a), f2=isfinite(b);
      if (__builtin_expect(f1 && f2, 1)) {
         if (__builtin_expect(!b.non_zero(), 0))
            throw GMP::ZeroDivide();
         return Rational(mpq_div,a.rep,b.rep);
      } else if (f1)
         return Rational();
      else if (!f2)
         throw GMP::NaN();
      return Rational(maximal<Rational>(), a.rep, mpq_sgn(b.rep)<0 ? -1 : 1);
   }

   /// Division
   friend Rational operator/ (const Rational& a, long b)
   {
      if (__builtin_expect(isfinite(a),1)) {
         if (__builtin_expect(!b, 0))
            throw GMP::ZeroDivide();
         if (__builtin_expect(!a.non_zero(), 0))
            return Rational();

         unsigned long g=mpz_gcd_ui(0, mpq_numref(a.rep), b>0 ? b : -b);
         if (g==1) {
            return Rational(mpq_numref(a.rep), mpz_mul_si, mpq_denref(a.rep), b, True());
         } else {
            return Rational(mpz_divexact_ui, mpq_numref(a.rep), g,
                            mpz_mul_si, mpq_denref(a.rep), b/(long)g, True());
         }
      }
      return Rational(maximal<Rational>(), a.rep, b<0 ? -1 : 1);
   }

   /// Division
   friend Rational operator/ (long a, const Rational& b)
   {
      if (__builtin_expect(isfinite(b),1)) {
         if (__builtin_expect(!b.non_zero(), 0))
            throw GMP::ZeroDivide();
         if (__builtin_expect(!a, 0))
            return Rational();

         unsigned long g=mpz_gcd_ui(0, mpq_numref(b.rep), a>0 ? a : -a);
         if (g==1) {
            return Rational(mpz_mul_si, mpq_denref(b.rep), a, mpq_numref(b.rep), True());
         } else {
            return Rational(mpz_mul_si, mpq_denref(b.rep), a/(long)g,
                            mpz_divexact_ui, mpq_numref(b.rep), g, True());
         }
      }
      return Rational();
   }

   /// Division
   friend Rational operator/ (const Rational& a, const Integer& b)
   {
      const bool f1=isfinite(a), f2=isfinite(b);
      if (__builtin_expect(f1 && f2, 1)) {
         if (__builtin_expect(!b.non_zero(), 0))
            throw GMP::ZeroDivide();
         if (!a.non_zero())
            return Rational();

         const Integer g=gcd(numerator(a), b);
         if (g==1) {
            return Rational(mpq_numref(a.rep), mpz_mul, mpq_denref(a.rep), b.get_rep(), True());
         } else {
            const Integer t=div_exact(b, g);
            return Rational(mpz_divexact, mpq_numref(a.rep), g.get_rep(),
                            mpz_mul, mpq_denref(a.rep), t.get_rep(), True());
         }
      } else if (f1)
         return Rational();
      else if (!f2)
         throw GMP::NaN();
      return Rational(maximal<Rational>(), a.rep, mpz_sgn(b.get_rep())<0 ? -1 : 1);
   }

   /// Division
   friend Rational operator/ (const Integer& a, const Rational& b)
   {
      const bool f1=isfinite(a), f2=isfinite(b);
      if (__builtin_expect(f1 && f2, 1)) {
         if (__builtin_expect(!b.non_zero(), 0))
            throw GMP::ZeroDivide();
         if (!a.non_zero())
            return Rational();
         const Integer g=gcd(a, numerator(b));
         if (g==1) {
            return Rational(mpz_mul, mpq_denref(b.rep), a.get_rep(), mpq_numref(b.rep), True());
         } else {
            const Integer t=div_exact(a, g);
            return Rational(mpz_mul, mpq_denref(b.rep), t.get_rep(),
                            mpz_divexact, mpq_numref(b.rep), g.get_rep(), True());
         }
      } else if (f1)
         return Rational();
      else if (!f2)
         throw GMP::NaN();
      return Rational(maximal<Rational>(), a.get_rep(), mpq_sgn(b.rep)<0 ? -1 : 1);
   }

   /// The floor function.
   friend Rational floor(const Rational& a)
   {
      if (__builtin_expect(isfinite(a),1))
         return Rational(mpz_fdiv_q, mpq_numref(a.rep), mpq_denref(a.rep), 1);
      return a;
   }

   /// The ceiling function.
   friend Rational ceil(const Rational& a)
   {
      if (__builtin_expect(isfinite(a),1))
         return Rational(mpz_cdiv_q, mpq_numref(a.rep), mpq_denref(a.rep), 1);
      return a;
   }

   /// Multiply with 2**k
   friend Rational operator<< (const Rational& a, unsigned long k)
   {
      if (__builtin_expect(isfinite(a),1))
         return Rational(mpq_mul_2exp, a.rep, k);
      return a;
   }

   /// Divide by 2**k
   friend Rational operator>> (const Rational& a, unsigned long k)
   {
      if (__builtin_expect(isfinite(a),1))
         return Rational(mpq_div_2exp, a.rep, k);
      return a;
   }

   friend int isfinite(const Rational& a);
   friend int isinf(const Rational& a);

   /// Comparison. 
   //The magnitude of the return value is arbitrary, only its sign is relevant.
   int compare(const Rational& b) const
   {
      const int i1=isinf(*this), i2=isinf(b);
      if (__builtin_expect(i1 || i2, 0))
         return i1-i2;
      return mpq_cmp(rep, b.rep);
   }

   /// Comparison. 
   int compare(long b) const
   {
      const int i1=isinf(*this);
      if (__builtin_expect(i1,0))
         return i1;
      return mpz_cmp_ui(mpq_denref(rep),1) ? numerator(*this).compare(b*denominator(*this))
                                           : numerator(*this).compare(b);
   }

   /// Comparison. 
   int compare(int b) const { return compare(long(b)); }

   /// Comparison. 
   int compare(const Integer& b) const
   {
      const int i1=isinf(*this), i2=isinf(b);
      if (__builtin_expect(i1 || i2, 0))
         return i1-i2;
      return mpz_cmp_ui(mpq_denref(rep),1) ? numerator(*this).compare(b*denominator(*this))
                                           : numerator(*this).compare(b);
   }

   /// Comparison. 
   int compare(double b) const
   {
      const int i1=isinf(*this), i2=isinf(b);
      if (__builtin_expect(i1 || i2,0))
         return i1-i2;
      if (!mpz_cmp_ui(mpq_denref(rep),1))
         return mpz_cmp_d(mpq_numref(rep), b);
      return sign(to_double()-b);
   }

   typedef list comparable_with(int, long, double, Integer);

   friend bool operator== (const Rational& a, const Rational& b)
   {
      if (__builtin_expect(isfinite(a) && isfinite(b), 1))
         return mpq_equal(a.rep, b.rep);
      return isinf(a)==isinf(b);
   }

   friend bool operator== (const Rational& a, const Integer& b)
   {
      if (__builtin_expect(isfinite(a) && isfinite(b), 1)) {
         return !mpz_cmp_ui(mpq_denref(a.rep),1) &&
            !mpz_cmp(mpq_numref(a.rep), b.get_rep());
      }
      return isinf(a)==isinf(b);
   }

   friend bool operator== (const Rational& a, long b)
   {
      return isfinite(a)
          && !mpz_cmp_ui(mpq_denref(a.rep),1)
          && mpz_fits_slong_p(mpq_numref(a.rep))
          && mpz_get_si(mpq_numref(a.rep))==b;
   }

   friend bool operator< (const Rational& a, const Integer& b)
   {
      if (__builtin_expect(isfinite(a) && isfinite(b), 1)) {
         if (!b.non_zero()) return mpq_sgn(a.rep)<0;
         return !mpz_cmp_ui(mpq_denref(a.rep),1) ? numerator(a) < b : numerator(a) < b*denominator(a);
      }
      return isinf(a) < isinf(b);
   }

   friend bool operator> (const Rational& a, const Integer& b)
   {
      if (__builtin_expect(isfinite(a) && isfinite(b), 1)) {
         if (!b.non_zero()) return mpq_sgn(a.rep)>0;
         return !mpz_cmp_ui(mpq_denref(a.rep),1) ? numerator(a) > b : numerator(a) > b*denominator(a);
      }
      return isinf(a) > isinf(b);
   }

   friend bool operator< (const Rational& a, long b)
   {
      const int i1=isinf(a);
      if (__builtin_expect(i1,0)) return i1<0;
      if (!b) return mpq_sgn(a.rep)<0;
      return !mpz_cmp_ui(mpq_denref(a.rep),1)
             ? mpz_cmp_si(mpq_numref(a.rep),b)<0
             : numerator(a) < b*denominator(a);
   }

   friend bool operator> (const Rational& a, long b)
   {
      const int i1=isinf(a);
      if (__builtin_expect(i1,0)) return i1>0;
      if (!b) return mpq_sgn(a.rep)>0;
      return !mpz_cmp_ui(mpq_denref(a.rep),1)
             ? mpz_cmp_si(mpq_numref(a.rep),b)>0
             : numerator(a) > b*denominator(a);
   }

   friend bool abs_equal(const Rational& a, const Rational& b)
   {
      const bool f1=isfinite(a), f2=isfinite(b);
      if (__builtin_expect(f1 && f2, 1))
         return !mpz_cmp(mpq_denref(a.rep), mpq_denref(b.rep)) &&
                !mpz_cmpabs(mpq_numref(a.rep), mpq_numref(b.rep));
      return f1==f2;
   }
   friend bool abs_equal(const Rational& a, const Integer& b)
   {
      const bool f1=isfinite(a), f2=isfinite(b);
      if (__builtin_expect(f1 && f2, 1))
         return !mpz_cmp_ui(mpq_denref(a.rep),1) &&
                !mpz_cmpabs(mpq_numref(a.rep), b.get_rep());
      return f1==f2;
   }
   friend bool abs_equal(const Rational& a, long b)
   {
      return isfinite(a) &&
             !mpz_cmp_ui(mpq_denref(a.rep),1) &&
             mpz_fits_slong_p(mpq_numref(a.rep)) && mpz_get_si(mpq_numref(a.rep))==std::abs(b);
   }

   friend Rational abs(const Rational& a)
   {
      if (__builtin_expect(isfinite(a),1))
         return Rational(mpz_abs, mpq_numref(a.rep), mpq_denref(a.rep));
      return Rational(maximal<Rational>(), 1);
   }

   friend Rational inv(const Rational& a)
   {
      if (__builtin_expect(isfinite(a), 1)) {
         if (__builtin_expect(!a.non_zero(), 0))
            return Rational(maximal<Rational>(), 1);
         return Rational(mpq_inv, a.rep);
      }
      return Rational();
   }


   /// Power.
   static Rational pow(const Rational& a, long k)
   {
      if (__builtin_expect(isfinite(a),1))
      {
         if (k > 0)
            return Rational(mpz_pow_ui, mpq_numref(a.rep), (unsigned long) k,
                            mpz_pow_ui, mpq_denref(a.rep), (unsigned long) k, False());
         else if (k < 0)
            return Rational(mpz_pow_ui, mpq_denref(a.rep), (unsigned long) abs(k),
                            mpz_pow_ui, mpq_numref(a.rep), (unsigned long) abs(k), True());
         else
            return Rational(1);
      }
      return Rational(maximal<Rational>(), k%2 ? isinf(a) : 1);
   }

   /// Power.
   static Rational pow(unsigned long a, long k)
   {
      if (k > 0)
         return Rational(mpz_ui_pow_ui, a, (unsigned long) k, 1);
      else if (k < 0)
         return Rational(1, mpz_ui_pow_ui, a, (unsigned long) abs(k));
      else
         return Rational(1);
   }

   /// Power
   static Rational pow(const Integer& a, long k)
   {
      if (__builtin_expect(isfinite(a),1))
      {
         if (k > 0)
            return Rational(mpz_pow_ui, a.rep, (unsigned long) k, 1);
         else if (k < 0)
            return Rational(1, mpz_pow_ui, a.rep, (unsigned long) abs(k));
         else
            return Rational(1);
      }
      return Rational(maximal<Rational>(), k%2 ? isinf(a) : 1);
   }



   void read(std::istream& is)
   {
      numerator_nocanon(*this).read(is);
      if (!is.eof() && is.peek() == '/') {
         is.ignore();
         denominator(*this).read(is,false);
         canonicalize();
      } else {
         mpz_set_ui(mpq_denref(rep), 1);
      }
   }

   friend
   std::istream& operator>> (std::istream& is, Rational& a)
   {
      a.read(is);
      return is;
   }

   friend
   std::ostream& operator<< (std::ostream& os, const Rational& a)
   {
      const std::ios::fmtflags flags=os.flags();
      bool show_den=false;
      int s=numerator(a).strsize(flags);
      if (mpz_cmp_ui(mpq_denref(a.rep),1)) {
         show_den=true;
         s+=denominator(a).strsize(flags);  // '/' occupies the place of the numerator's terminating NULL
      }
      a.putstr(flags, OutCharBuffer::reserve(os,s), show_den);
      return os;
   }

   void putstr(std::ios::fmtflags flags, char *buf, bool show_den) const;

#if POLYMAKE_DEBUG
   void dump() const { std::cerr << *this << std::flush; }
#endif
};

}
namespace std {

inline void swap(pm::Rational& a, pm::Rational& b) { a.swap(b); }

template <>
class numeric_limits<pm::Rational> : public numeric_limits<pm::Integer> {
public:
   static const bool is_integer=false;
   static pm::Rational min() throw() { return pm::Rational(pm::maximal<pm::Rational>(),-1); }
   static pm::Rational infinity() throw() { return pm::Rational(pm::maximal<pm::Rational>()); }
   static pm::Rational max() throw() { return pm::Rational(pm::maximal<pm::Rational>()); }
};

}
namespace pm {

inline Rational operator+ (const Rational& a) { return a; }

inline bool operator!= (const Rational& a, const Rational& b) { return !(a==b); }
inline bool operator< (const Rational& a, const Rational& b) { return a.compare(b)<0; }
inline bool operator> (const Rational& a, const Rational& b) { return a.compare(b)>0; }
inline bool operator<= (const Rational& a, const Rational& b) { return a.compare(b)<=0; }
inline bool operator>= (const Rational& a, const Rational& b) { return a.compare(b)>=0; }

namespace GMP {
inline bool operator== (const TempRational& a, const TempRational& b) { return mpq_equal(&a,&b); }
inline bool operator!= (const TempRational& a, const TempRational& b) { return !(a==b); }
inline bool operator< (const TempRational& a, const TempRational& b) { return mpq_cmp(&a,&b)<0; }
inline bool operator> (const TempRational& a, const TempRational& b) { return mpq_cmp(&a,&b)>0; }
inline bool operator<= (const TempRational& a, const TempRational& b) { return mpq_cmp(&a,&b)<=0; }
inline bool operator>= (const TempRational& a, const TempRational& b) { return mpq_cmp(&a,&b)>=0; }

template <proxy_kind kind, bool _canonicalize> inline
void Proxy<kind, _canonicalize>::canonicalize()
{
   if (!_canonicalize) return;

   // The constant 8 is arbitrarily chosen.
   // Every non-zero double word aligned address would do as well.
   enum { fict_addr=8 };

   Rational *me=reinterpret_cast<Rational*>
      (// address of the num/den field
       reinterpret_cast<char*>(this)
       - (// offset from the begin of a mpq_t to appropriate mpz_t
          reinterpret_cast<char*>(kind==num ? mpq_numref(reinterpret_cast<Rational*>(fict_addr)->rep)
                                  : mpq_denref(reinterpret_cast<Rational*>(fict_addr)->rep))
          -reinterpret_cast<char*>(fict_addr)));
   if (kind==num) {
      if (__builtin_expect(isfinite(*this),1))
         me->canonicalize();
      else
         mpz_set_ui(mpq_denref(me->rep), 1);
   } else {
      if (__builtin_expect(isfinite(*me),1)) {
         if (__builtin_expect(isfinite(*this),1))
            me->canonicalize();
         else {
            mpz_set_ui(mpq_numref(me->rep), 0);
            mpz_init_set_ui(rep, 1);
         }
      } else if (__builtin_expect(isfinite(*this),1)) {
         mpz_set_ui(rep, 1);
      } else {
         throw NaN();
      }
   }
}
}

inline bool operator== (const Integer& a, const Rational& b) { return b==a; }
inline bool operator== (long a, const Rational& b) { return b==a; }
inline bool operator== (const Rational& a, int b) { return a==long(b); }
inline bool operator== (int a, const Rational& b) { return b==long(a); }
inline bool operator== (const Rational& a, double b) { return a.to_double()==b; }
inline bool operator== (double a, const Rational& b) { return b.to_double()==a; }

inline bool abs_equal(const Integer& a, const Rational& b) { return abs_equal(b,a); }
inline bool abs_equal(long a, const Rational& b) { return abs_equal(b,a); }
inline bool abs_equal(const Rational& a, int b) { return abs_equal(a,long(b)); }
inline bool abs_equal(int a, const Rational& b) { return abs_equal(b,long(a)); }
inline bool abs_equal(const Rational& a, double b) { const double da=a.to_double(); return da==b || da==-b; }
inline bool abs_equal(double a, const Rational& b) { const double db=b.to_double(); return db==a || db==-a; }

inline bool operator!= (const Rational& a, const Integer& b) { return !(a==b); }
inline bool operator!= (const Integer& a, const Rational& b) { return !(b==a); }
inline bool operator!= (const Rational& a, long b) { return !(a==b); }
inline bool operator!= (long a, const Rational& b) { return !(b==a); }
inline bool operator!= (const Rational& a, int b) { return !(a==b); }
inline bool operator!= (int a, const Rational& b) { return !(b==a); }
inline bool operator!= (const Rational& a, double b) { return !(a==b); }
inline bool operator!= (double a, const Rational& b) { return !(b==a); }

inline bool operator< (const Rational& a, int b) { return a < long(b); }
inline bool operator< (const Rational& a, double b) { return a.to_double() < b; }
inline bool operator< (double a, const Rational& b) { return a < b.to_double(); }

inline bool operator> (const Rational& a, int b) { return a > long(b); }
inline bool operator> (const Rational& a, double b) { return a.to_double() > b; }
inline bool operator> (double a, const Rational& b) { return a > b.to_double(); }

inline bool operator< (const Integer& a, const Rational& b) { return b>a; }
inline bool operator< (long a, const Rational& b) { return b>a; }
inline bool operator< (int a, const Rational& b) { return b>long(a); }
inline bool operator> (const Integer& a, const Rational& b) { return b<a; }
inline bool operator> (long a, const Rational& b) { return b<a; }
inline bool operator> (int a, const Rational& b) { return b<long(a); }

inline bool operator<= (const Rational& a, const Integer& b) { return !(a>b); }
inline bool operator<= (const Integer& a, const Rational& b) { return !(b<a); }

inline bool operator<= (const Rational& a, long b) { return !(a>b); }
inline bool operator<= (const Rational& a, int b) { return !(a>long(b)); }
inline bool operator<= (long a, const Rational& b) { return !(b<a); }
inline bool operator<= (int a, const Rational& b) { return !(b<long(a)); }
inline bool operator<= (const Rational& a, double b) { return !(a>b); }
inline bool operator<= (double a, const Rational& b) { return !(b<a); }

inline bool operator>= (const Rational& a, const Integer& b) { return !(a<b); }
inline bool operator>= (const Rational& a, long b) { return !(a<b); }
inline bool operator>= (const Rational& a, int b) { return !(a<long(b)); }
inline bool operator>= (const Integer& a, const Rational& b) { return !(b>a); }
inline bool operator>= (long a, const Rational& b) { return !(b>a); }
inline bool operator>= (int a, const Rational& b) { return !(b>long(a)); }
inline bool operator>= (const Rational& a, double b) { return !(a<b); }
inline bool operator>= (double a, const Rational& b) { return !(b>a); }

Integer& Integer::operator= (const Rational& b)
{
   if (__builtin_expect(isfinite(b), 1)) {
      if (__builtin_expect(!isfinite(*this), 0)) mpz_init(rep);
      if (! mpz_cmp_ui(mpq_denref(b.rep),1))
         mpz_set(rep,mpq_numref(b.rep));
      else
         mpz_tdiv_q(rep, mpq_numref(b.rep), mpq_denref(b.rep));
   } else
      _set_inf(rep, mpq_numref(b.rep));
   return *this;
}

Integer::Integer(const Rational& b)
{
   if (__builtin_expect(isfinite(b), 1)) {
      if (! mpz_cmp_ui(mpq_denref(b.rep),1)) {
         mpz_init_set(rep,mpq_numref(b.rep));
      } else {
         mpz_init(rep);
         mpz_tdiv_q(rep, mpq_numref(b.rep), mpq_denref(b.rep));
      }
   } else
      _init_set_inf(rep, mpq_numref(b.rep));
}

Rational& Rational::operator*= (const Integer& b)
{
   const bool f1=isfinite(*this), f2=isfinite(b);
   if (__builtin_expect(f1 && f2, 1)) {
      if (__builtin_expect(!non_zero(), 0)) return *this;
      if (__builtin_expect(mpz_sgn(b.rep), 1)) {
         Integer g=gcd(denominator(*this), b);
         if (g==1) {
            mpz_mul(mpq_numref(rep), mpq_numref(rep), b.rep);
         } else {
            mpz_divexact(mpq_denref(rep), mpq_denref(rep), g.rep);
            mpz_divexact(g.rep, b.rep, g.rep);
            mpz_mul(mpq_numref(rep), mpq_numref(rep), g.rep);
         }
      } else {
         *this=0;
      }
   } else
      _inf_inv_sign(rep,mpz_sgn(b.rep));
   return *this;
}

Rational& Rational::operator/= (const Integer& b)
{
   const bool f1=isfinite(*this), f2=isfinite(b);
   if (__builtin_expect(f1 && f2, 1)) {
      if (__builtin_expect(!b.non_zero(), 0))
         throw GMP::ZeroDivide();
      if (__builtin_expect(mpq_sgn(rep), 1)) {
         Integer g=gcd(numerator(*this), b);
         if (g==1) {
            mpz_mul(mpq_denref(rep), mpq_denref(rep), b.rep);
         } else {
            mpz_divexact(mpq_numref(rep), mpq_numref(rep), g.rep);
            mpz_divexact(g.rep, b.rep, g.rep);
            mpz_mul(mpq_denref(rep), mpq_denref(rep), g.rep);
         }
         canonicalize_sign();
      }
   } else if (f1)
      *this=0;
   else if (f2)
      _inf_inv_sign(rep,mpz_sgn(b.rep),true);
   else
      throw GMP::NaN();
   return *this;
}

inline Rational operator+ (const Rational& a, int b) { return a+long(b); }
inline Rational operator+ (const Integer& a, const Rational& b) { return b+a; }
inline Rational operator+ (long a, const Rational& b) { return b+a; }
inline Rational operator+ (int a, const Rational& b) { return b+long(a); }

inline Rational operator- (const Rational& a, int b) { return a-long(b); }
inline Rational operator- (int a, const Rational& b) { return long(a)-b; }

inline Rational operator* (const Rational& a, const Integer& b)
{
   const bool f1=isfinite(a), f2=isfinite(b);
   if (__builtin_expect(f1 && f2, 1)) {
      if (__builtin_expect(!a.non_zero() || !b.non_zero(), 0)) return Rational();
      const Integer g=gcd(denominator(a),b);
      if (g==1) {
         return Rational(mpz_mul, mpq_numref(a.rep), b.get_rep(), mpq_denref(a.rep), False());
      } else {
         const Integer t=div_exact(b,g);
         return Rational(mpz_mul, mpq_numref(a.rep), t.get_rep(),
                         mpz_divexact, mpq_denref(a.rep), g.get_rep(), False());
      }
   }
   const int s=mpq_sgn(a.rep)*mpz_sgn(b.get_rep());
   if (!s) throw GMP::NaN();
   return Rational(maximal<Rational>(), s);
}

inline Rational operator* (const Rational& a, int b) { return a*long(b); }
inline Rational operator* (const Integer& a, const Rational& b) { return b*a; }
inline Rational operator* (long a, const Rational& b) { return b*a; }
inline Rational operator* (int a, const Rational& b) { return b*long(a); }

inline Rational operator/ (const Rational& a, int b) { return a/long(b); }
inline Rational operator/ (int a, const Rational& b) { return long(a)/b; }

inline Rational operator<< (const Rational& a, unsigned int k)
{
   return a << static_cast<unsigned long>(k);
}

inline Rational operator>> (const Rational& a, unsigned int k)
{
   return a >> static_cast<unsigned long>(k);
}

inline Rational operator<< (const Rational& a, long k)
{
   if (k<0) return a >> static_cast<unsigned long>(-k);
   return a << static_cast<unsigned long>(k);
}

inline Rational operator>> (const Rational& a, long k)
{
   if (k<0) return a << static_cast<unsigned long>(-k);
   return a >> static_cast<unsigned long>(k);
}

inline Rational operator<< (const Rational& a, int k) { return a << long(k); }
inline Rational operator>> (const Rational& a, int k) { return a >> long(k); }

template <> inline
cmp_value sign(const Rational& a) { return sign(mpq_numref(a.get_rep())->_mp_size); }

int isfinite(const Rational& a)
{
   return mpq_numref(a.rep)->_mp_alloc;
}

int isinf(const Rational& a)
{
   return isfinite(a) ? 0 : mpq_numref(a.rep)->_mp_size;
}

namespace operations {

template <>
struct cmp_scalar<Rational, Rational, true> : cmp_GMP_based<Rational> {};

} // end namespace operations

template <>
class conv<Rational, int> {
public:
   typedef Rational argument_type;
   typedef int result_type;
   result_type operator() (const Rational& a) const { return a.to_int(); }
};

template <>
class conv<Rational, long> {
public:
   typedef Rational argument_type;
   typedef long result_type;
   result_type operator() (const Rational& a) const { return a.to_long(); }
};

template <>
class conv<Rational, double> {
public:
   typedef Rational argument_type;
   typedef double result_type;
   result_type operator() (const Rational& a) const { return a.to_double(); }
};

template <>
class conv<Rational, std::string> {
public:
   typedef Rational argument_type;
   typedef std::string result_type;
   result_type operator() (const Rational& a) const { return a.to_string(); }
};

template <>
class conv<Rational, Integer> : public conv_by_cast<Rational, Integer> {};

template <GMP::proxy_kind kind, bool _canonicalize>
struct spec_object_traits< GMP::Proxy<kind, _canonicalize> > : spec_object_traits<Integer> {
   typedef Integer persistent_type;
   typedef Integer masquerade_for;
};

template <GMP::proxy_kind kind, bool _canonicalize>
struct const_equivalent< GMP::Proxy<kind, _canonicalize> > {
   typedef Integer type;
};

template <GMP::proxy_kind kind, bool _canonicalize, class Target>
class conv<GMP::Proxy<kind, _canonicalize>, Target> :
   public conv<Integer, Target> {
public:
   typedef GMP::Proxy<kind, _canonicalize> argument_type;
};

template <>
struct spec_object_traits<GMP::TempRational> : spec_object_traits<is_scalar> {};

template <>
class conv<GMP::TempRational, Rational> : public conv_by_cast<GMP::TempRational&, Rational> {};

template <>
class conv<GMP::TempInteger, Rational> : public conv_by_cast<GMP::TempInteger&, Rational> {};

template <>
struct spec_object_traits<Rational> : spec_object_traits<is_scalar> {
   static
   bool is_zero(const Rational& a)
   {
      return !a.non_zero();
   }

   static
   bool is_one(const Rational& a)
   {
      return a==1L;
   }

   static const Rational& zero();
   static const Rational& one();
};

template <>
struct hash_func<MP_RAT, is_opaque> : hash_func<MP_INT> {
protected:
   size_t _do(mpq_srcptr a) const
   {
      return hash_func<MP_INT>::_do(mpq_numref(a)) - hash_func<MP_INT>::_do(mpq_denref(a));
   }
public:
   size_t operator() (const MP_RAT& a) const { return _do(&a); }
};

template <>
struct hash_func<Rational, is_scalar> : hash_func<MP_RAT> {
   size_t operator() (const Rational& a) const { return __builtin_expect(isfinite(a),1) ? _do(a.get_rep()) : 0; }
};

template <>
struct hash_func<GMP::TempRational, is_scalar> : hash_func<MP_RAT> {
   size_t operator() (const GMP::TempRational& a) const { return hash_func<MP_RAT>::operator()(reinterpret_cast<const MP_RAT&>(a)); }
};

template <>
struct algebraic_traits<int> {
   typedef Rational field_type;
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

namespace GMP {

inline
std::ostream& operator<< (std::ostream& os, const GMP::TempRational& a)
{
   return os << reinterpret_cast<const Rational&>(a);
}

} }
namespace polymake {
   using pm::Rational;
}

#endif // POLYMAKE_RATIONAL_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
