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

#ifndef POLYMAKE_RATIONAL_FUNCTION_H
#define POLYMAKE_RATIONAL_FUNCTION_H

#include "polymake/Polynomial.h"

namespace pm {

template <typename T, typename Coefficient, typename Exponent>
struct is_unipolynomial_type {
   static const bool value=derived_from<T, UniMonomial<Coefficient, Exponent> >::value ||
                           derived_from<T, UniTerm<Coefficient, Exponent> >::value ||
                           derived_from<T, UniPolynomial<Coefficient, Exponent> >::value;
};

template <typename Coefficient, typename Exponent>
class RationalFunction {
public:
   typedef UniPolynomial<Coefficient, Exponent> polynomial_type;
   typedef typename polynomial_type::term_hash term_hash;
   typedef typename polynomial_type::ring_type ring_type;
   typedef typename polynomial_type::term_type term_type;
   typedef typename polynomial_type::monomial_type monomial_type;
   typedef Coefficient coefficient_type;
   typedef Exponent exponent_type;

   /// Construct a zero value.
   RationalFunction() :
      num(),
      den(num.get_ring().one_coef(), num.get_ring()) {}

   /// Construct a zero value of explicitely given ring type.
   explicit RationalFunction(const ring_type& r) :
      num(r),
      den(r.one_coef(), r) {}

   /// Construct a constant value.
   template <typename T>
   explicit RationalFunction(const T& c,
                             typename enable_if<void**, fits_as_coefficient<T, polynomial_type>::value>::type=0) :
      num(c),
      den(num.get_ring().one_coef(), num.get_ring()) {}

   /// Construct a constant value of explicitely given ring type.
   template <typename T>
   RationalFunction(const T& c, const ring_type& r,
                    typename enable_if<void**, fits_as_coefficient<T, polynomial_type>::value>::type=0) :
      num(c, r),
      den(r.one_coef(), r) {}

   /// Construct a value with denominator equal to 1.
   template <typename T>
   explicit RationalFunction(const T& p,
                             typename enable_if<void**, (is_unipolynomial_type<T, Coefficient, Exponent>::value)>::type=0) :
      num(p),
      den(num.get_ring().one_coef(), num.get_ring()) {}

   /// Construct a constant value expressed as ratio of two scalars.
   template <typename T1, typename T2>
   RationalFunction(const T1& c1, const T2& c2,
                    typename enable_if<void**, (fits_as_coefficient<T1, polynomial_type>::value && fits_as_coefficient<T2, polynomial_type>::value)>::type=0) :
      num(c1),
      den(num.get_ring().one_coef(), num.get_ring())
   {
      num /= c2;
   }

   /// Construct a constant value expressed as ratio of two scalars, of an explicitely given ring type.
   template <typename T1, typename T2>
   RationalFunction(const T1& c1, const T2& c2, const ring_type& r,
                    typename enable_if<void**, (fits_as_coefficient<T1, polynomial_type>::value && fits_as_coefficient<T2, polynomial_type>::value)>::type=0) :
      num(c1, r),
      den(r.one_coef(), r)
   {
      num /= c2;
   }

   /// Construct a value with denominator equal to a constant scalar.
   template <typename T1, typename T2>
   RationalFunction(const T1& p1, const T2& c2,
                    typename enable_if<void**, (is_unipolynomial_type<T1, Coefficient, Exponent>::value && fits_as_coefficient<T2, polynomial_type>::value)>::type=0) :
      num(p1),
      den(num.get_ring().one_coef(), num.get_ring())
   {
      num /= c2;
   }

   /// Construct a value with numerator equal to a constant scalar.
   template <typename T1, typename T2>
   RationalFunction(const T1& c1, const T2& p2,
                    typename enable_if<void**, (fits_as_coefficient<T1, polynomial_type>::value && is_unipolynomial_type<T2, Coefficient, Exponent>::value)>::type=0) :
      num(c1, p2.get_ring()),
      den(p2)
   {
      if (is_zero(p2)) throw GMP::ZeroDivide();
      normalize_lc();
   }

   /// Construct a value with given numerator and denominator.
   template <typename T1, typename T2>
   RationalFunction(const T1& p1, const T2& p2,
                    typename enable_if<void**, (is_unipolynomial_type<T1, Coefficient, Exponent>::value && is_unipolynomial_type<T2, Coefficient, Exponent>::value)>::type=0)
   {
      if (p1.get_ring() != p2.get_ring()) throw std::runtime_error("RationalFunction - arguments of different rings");
      if (is_zero(p2)) throw GMP::ZeroDivide();
      simplify(p1, p2);
      normalize_lc();
   }

#if POLYMAKE_DEBUG
   ~RationalFunction() { POLYMAKE_DEBUG_METHOD(RationalFunction,dump); }
   void dump() const { cerr << *this << std::flush; }
#endif

   const ring_type& get_ring() const { return num.get_ring(); }

   typedef polynomial_type numerator_type;
   typedef polynomial_type denominator_type;

   friend
   const polynomial_type& numerator(const RationalFunction& me) { return me.num; }

   friend
   const polynomial_type& denominator(const RationalFunction& me) { return me.den; }

   void swap(RationalFunction& other)
   {
      num.swap(other.num);
      den.swap(other.den);
   }

   RationalFunction& negate()
   {
      num.negate();
      return *this;
   }

   RationalFunction operator- () const
   {
      return RationalFunction(-num, den, True());
   }

   template <typename T>
   typename enable_if<RationalFunction, (fits_as_coefficient<T, polynomial_type>::value || is_unipolynomial_type<T, Coefficient, Exponent>::value)>::type&
   operator+= (const T& c)
   {
      if (__builtin_expect(!is_zero(c), 1))
         num += den * c;
      return *this;
   }

   RationalFunction& operator+= (const RationalFunction& rf)
   {
      if (__builtin_expect(!rf.num.trivial(), 1)) {
         ExtGCD<polynomial_type> x = ext_gcd(den, rf.den, false);
         x.k1 *= rf.num;  x.k1 += num * x.k2;
         x.k2 *= den;
         x = ext_gcd(x.k1, x.k2);
         num.swap(x.k1);
         den.swap(x.k2);
         normalize_lc();
      }
      return *this;
   }

   template <typename T>
   typename enable_if<RationalFunction, (fits_as_coefficient<T, polynomial_type>::value || is_unipolynomial_type<T, Coefficient, Exponent>::value)>::type&
   operator-= (const T& c)
   {
      if (__builtin_expect(!is_zero(c), 1))
         num -= den * c;
      return *this;
   }

   RationalFunction& operator-= (const RationalFunction& rf)
   {
      if (__builtin_expect(!rf.num.trivial(), 1)) {
         ExtGCD<polynomial_type> x = ext_gcd(den, rf.den, false);
         x.k1 *= rf.num;  x.k1.negate();  x.k1 += num * x.k2;
         x.k2 *= den;
         x = ext_gcd(x.k1, x.k2);
         num.swap(x.k1);
         den.swap(x.k2);
         normalize_lc();
      }
      return *this;
   }

   template <typename T> friend
   typename enable_if<RationalFunction, (fits_as_coefficient<T, polynomial_type>::value || is_unipolynomial_type<T, Coefficient, Exponent>::value)>::type
   operator+ (const RationalFunction& rf, const T& c)
   {
      if (__builtin_expect(!is_zero(c), 1))
         return RationalFunction(rf.num + rf.den * c, rf.den, True());
      else
         return rf;
   }

   template <typename T> friend
   typename enable_if<RationalFunction, (fits_as_coefficient<T, polynomial_type>::value || is_unipolynomial_type<T, Coefficient, Exponent>::value)>::type
   operator+ (const T& c, const RationalFunction& rf)
   {
      return rf+c;
   }

   template <typename T> friend
   typename enable_if<RationalFunction, (fits_as_coefficient<T, polynomial_type>::value || is_unipolynomial_type<T, Coefficient, Exponent>::value)>::type
   operator- (const RationalFunction& rf, const T& c)
   {
      if (__builtin_expect(!is_zero(c), 1))
         return RationalFunction(rf.num - rf.den * c, rf.den, True());
      else
         return rf;
   }

   template <typename T> friend
   typename enable_if<RationalFunction, (fits_as_coefficient<T, polynomial_type>::value || is_unipolynomial_type<T, Coefficient, Exponent>::value)>::type
   operator- (const T& c, const RationalFunction& rf)
   {
      if (__builtin_expect(!is_zero(c), 1))
         return RationalFunction(rf.den * c - rf.num, rf.den, True());
      else
         return -rf;
   }

   friend
   RationalFunction operator+ (const RationalFunction& rf1, const RationalFunction& rf2)
   {
      if (__builtin_expect(rf1.num.trivial(), 0)) {
         return rf2;
      } else if (__builtin_expect(rf2.num.trivial(), 0)) {
         return rf1;
      } else {
         const ExtGCD<polynomial_type> x = ext_gcd(rf1.den, rf2.den, false);
         return RationalFunction(rf1.num * x.k2 + rf2.num * x.k1, rf1.den * x.k2);
      }
   }

   friend
   RationalFunction operator- (const RationalFunction& rf1, const RationalFunction& rf2)
   {
      if (__builtin_expect(rf1.num.trivial(), 0)) {
         return -rf2;
      } else if (__builtin_expect(rf2.num.trivial(), 0)) {
         return rf1;
      } else {
         const ExtGCD<polynomial_type> x = ext_gcd(rf1.den, rf2.den, false);
         return RationalFunction(rf1.num * x.k2 - rf2.num * x.k1, rf1.den * x.k2);
      }
   }

   template <typename T>
   typename enable_if<RationalFunction, fits_as_coefficient<T, polynomial_type>::value>::type&
   operator*= (const T& c)
   {
      num *= c;
      return *this;
   }

   template <typename T>
   typename enable_if<RationalFunction, fits_as_coefficient<T, polynomial_type>::value>::type&
   operator/= (const T& c)
   {
      num /= c;
      return *this;
   }

   template <typename T> friend
   typename enable_if<RationalFunction, fits_as_coefficient<T, polynomial_type>::value>::type
   operator* (const RationalFunction& rf, const T& c)
   {
      if (__builtin_expect(!is_zero(c), 1))
         return RationalFunction(rf.num * c, rf.den, True());
      else
         return RationalFunction(rf.get_ring());
   }

   template <typename T> friend
   typename enable_if<RationalFunction, fits_as_coefficient<T, polynomial_type>::value>::type
   operator* (const T& c, const RationalFunction& rf)
   {
      return rf * c;
   }

   template <typename T> friend
   typename enable_if<RationalFunction, fits_as_coefficient<T, polynomial_type>::value>::type
   operator/ (const RationalFunction& rf, const T& c)
   {
      return RationalFunction(rf.num / c, rf.den, True());
   }

   template <typename T> friend
   typename enable_if<RationalFunction, fits_as_coefficient<T, polynomial_type>::value>::type
   operator/ (const T& c, const RationalFunction& rf)
   {
      if (__builtin_expect(rf.num.trivial(), 0)) {
         throw GMP::ZeroDivide();
      } else if (__builtin_expect(is_zero(c), 0)) {
         return RationalFunction(rf.get_ring());
      } else {
         return RationalFunction(rf.den * c, rf.num, False());
      }
   }

   template <typename T> friend
   typename enable_if<RationalFunction, is_unipolynomial_type<T, Coefficient, Exponent>::value>::type
   operator* (const RationalFunction& rf, const T& p)
   {
      if (__builtin_expect(is_zero(p), 1)) {
         return RationalFunction(rf.get_ring());
      } else if (__builtin_expect(rf.num.trivial(), 0)) {
         return rf;
      } else {
         const ExtGCD<polynomial_type> x = ext_gcd(rf.den, p, false);
         return RationalFunction(rf.num * x.k2, x.k1, True());
      }
   }

   template <typename T> friend
   typename enable_if<RationalFunction, is_unipolynomial_type<T, Coefficient, Exponent>::value>::type
   operator* (const T& p, const RationalFunction& rf)
   {
      return rf * p;
   }

   friend
   RationalFunction operator* (const RationalFunction& rf1, const RationalFunction& rf2)
   {
      if (__builtin_expect(rf1.num.trivial(), 0)) {
         return rf1;
      } else if (__builtin_expect(rf2.num.trivial(), 0)) {
         return rf2;
      } else {
         const ExtGCD<polynomial_type> x = ext_gcd(rf1.num, rf2.den, false),
                                       y = ext_gcd(rf1.den, rf2.num, false);
         return RationalFunction(x.k1 * y.k2, y.k1 * x.k2, False());
      }
   }

   template <typename T> friend
   typename enable_if<RationalFunction, is_unipolynomial_type<T, Coefficient, Exponent>::value>::type
   operator/ (const RationalFunction& rf, const T& p)
   {
      if (__builtin_expect(is_zero(p), 1)) {
         throw GMP::ZeroDivide();
      } else if (__builtin_expect(rf.num.trivial(), 0)) {
         return rf;
      } else {
         const ExtGCD<polynomial_type> x = ext_gcd(rf.num, p, false);
         return RationalFunction(x.k1, rf.den * x.k2, False());
      }
   }

   template <typename T> friend
   typename enable_if<RationalFunction, is_unipolynomial_type<T, Coefficient, Exponent>::value>::type
   operator/ (const T& p, const RationalFunction& rf)
   {
      if (__builtin_expect(rf.num.trivial(), 1)) {
         throw GMP::ZeroDivide();
      } else if (__builtin_expect(is_zero(p), 0)) {
         return RationalFunction(rf.get_ring());
      } else {
         const ExtGCD<polynomial_type> x = ext_gcd(rf.num, p, false);
         return RationalFunction(rf.den * x.k2, x.k1, False());
      }
   }

   friend
   RationalFunction operator/ (const RationalFunction& rf1, const RationalFunction& rf2)
   {
      if (__builtin_expect(rf2.num.trivial(), 0)) {
         throw GMP::ZeroDivide();
      } else if (__builtin_expect(rf1.num.trivial(), 0)) {
         return rf1;
      } else {
         const ExtGCD<polynomial_type> x = ext_gcd(rf1.num, rf2.num, false),
                                       y = ext_gcd(rf1.den, rf2.den, false);
         return RationalFunction(x.k1 * y.k2, y.k1 * x.k2, False());
      }
   }

   template <typename T>
   typename enable_if<RationalFunction, is_unipolynomial_type<T, Coefficient, Exponent>::value>::type&
   operator*= (const T& p)
   {
      *this = (*this) * p;
      return *this;
   }

   template <typename T>
   typename enable_if<RationalFunction, is_unipolynomial_type<T, Coefficient, Exponent>::value>::type&
   operator/= (const T& p)
   {
      *this = (*this) / p;
      return *this;
   }

   RationalFunction& operator*= (const RationalFunction& rf)
   {
      *this = (*this) * rf;
      return *this;
   }

   RationalFunction& operator/= (const RationalFunction& rf)
   {
      *this = (*this) / rf;
      return *this;
   }

   friend
   bool operator== (const RationalFunction& rf1, const RationalFunction& rf2)
   {
      return rf1.num == rf2.num && rf1.den == rf2.den;
   }

   template <typename T> friend
   typename enable_if<bool, (fits_as_coefficient<T, polynomial_type>::value || is_unipolynomial_type<T, Coefficient, Exponent>::value)>::type
   operator== (const RationalFunction& rf, const T& c)
   {
      return rf.den.unit() && rf.num == c;
   }

   template <typename T> friend
   typename enable_if<bool, (fits_as_coefficient<T, polynomial_type>::value || is_unipolynomial_type<T, Coefficient, Exponent>::value)>::type
   operator== (const T& c, const RationalFunction& rf)
   {
      return rf == c;
   }

   friend
   bool operator!= (const RationalFunction& rf1, const RationalFunction& rf2)
   {
      return !(rf1 == rf2);
   }

   template <typename T> friend
   typename enable_if<bool, (fits_as_coefficient<T, polynomial_type>::value || is_unipolynomial_type<T, Coefficient, Exponent>::value)>::type
   operator!= (const RationalFunction& rf, const T& c)
   {
      return !(rf == c);
   }

   template <typename T> friend
   typename enable_if<bool, (fits_as_coefficient<T, polynomial_type>::value || is_unipolynomial_type<T, Coefficient, Exponent>::value)>::type
   operator!= (const T& c, const RationalFunction& rf)
   {
      return !(rf == c);
   }

   template <typename Output> friend
   Output& operator<< (GenericOutput<Output>& out, const RationalFunction& rf)
   {
      out.top() << '(' << rf.num << ")/(" << rf.den << ')';
      return out.top();
   }

   UniPolynomial<Coefficient, Exponent> to_unipolynomial() const { 
      if (!den.unit())
         throw std::runtime_error("Denominator is not one; cannot convert to a polynomial");
      return num;
   }



protected:
   polynomial_type num, den;

   /// internally used constructor, when the operands are known to be mutually prime and normalized
   template <bool lc_normalized>
   RationalFunction(const polynomial_type& num_arg, const polynomial_type& den_arg, bool2type<lc_normalized>) :
      num(num_arg),
      den(den_arg)
   {
      if (!lc_normalized) normalize_lc();
   }

   void normalize_lc()
   {
      if (num.trivial()) {
         den=polynomial_type(get_ring().one_coef(), get_ring());
      } else {
         const Coefficient den_lc=den.lc();
         if (!is_one(den_lc)) {
            num /= den_lc;
            den /= den_lc;
         }
      }
   }

   // constructor helpers

   void simplify(const Coefficient& c1, const Exponent& e1,
                 const Coefficient& c2, const Exponent& e2,
                 const ring_type& ring)
   {
      if (e1 < e2)
      {
         // x^e1 / x^e2  ==  1 / x^(e2-e1)
         num=term_type(c1, ring);
         den=term_type(monomial_type(e2-e1, ring), c2);
      }
      else
      {
         // x^e1 / x^e2  ==  x^(e1-e2)
         num=term_type(monomial_type(e1-e2, ring), c1);
         den=term_type(c2, ring);
      }
   }

   void simplify(const polynomial_type& p1,
                 const Coefficient& c2, const Exponent& e2,
                 const ring_type& ring)
   {
      const Exponent e1=p1.lower_deg();
      if (e1 < e2)
      {
         // r(x)*x^e1 / x^e2  ==  r(x) / x^(e2-e1)
         if ( !is_zero(e1) ) {
            div_exact(p1, monomial_type(e1, ring)).swap(num);
         } else {
            num=p1;
         }
         den=term_type(monomial_type(e2-e1, ring), c2);
      }
      else
      {
         // r(x)*x^e1 / x^e2  ==  r(x)*x^(e1-e2)
         div_exact(p1, monomial_type(e2, ring)).swap(num);
         den=term_type(c2, ring);
      }
   }

   void simplify(const monomial_type& p1, const monomial_type& p2)
   {
      const ring_type& ring=p1.get_ring();
      simplify(ring.one_coef(), p1.get_value(), ring.one_coef(), p2.get_value(), ring);
   }

   void simplify(const monomial_type& p1, const term_type& p2)
   {
      const ring_type& ring=p1.get_ring();
      simplify(ring.one_coef(), p1.get_value(), p2.get_value().second, p2.get_value().first, ring);
   }

   void simplify(const term_type& p1, const monomial_type& p2)
   {
      const ring_type& ring=p1.get_ring();
      simplify(p1.get_value().second, p1.get_value().first, ring.one_coef(), p2.get_value(), ring);
   }

   void simplify(const term_type& p1, const term_type& p2)
   {
      const ring_type& ring=p1.get_ring();
      simplify(p1.get_value().second, p1.get_value().first, p2.get_value().second, p2.get_value().first, ring);
   }

   void simplify(const polynomial_type& p1, const monomial_type& p2)
   {
      const ring_type& ring=p1.get_ring();
      simplify(p1, ring.one_coef(), p2.get_value(), ring);
   }

   void simplify(const polynomial_type& p1, const term_type& p2)
   {
      const ring_type& ring=p1.get_ring();
      simplify(p1, p2.get_value().second, p2.get_value().first, ring);
   }

   void simplify(const monomial_type& p1, const polynomial_type& p2)
   {
      const ring_type& ring=p1.get_ring();
      simplify(p2, ring.one_coef(), p1.get_value(), ring);
      num.swap(den);
   }

   void simplify(const term_type& p1, const polynomial_type& p2)
   {
      const ring_type& ring=p1.get_ring();
      simplify(p2, p1.get_value().second, p1.get_value().first, ring);
      num.swap(den);
   }

   void simplify(const polynomial_type& p1, const polynomial_type& p2)
   {
      ExtGCD<polynomial_type> x=ext_gcd(p1, p2, false);
      num.swap(x.k1);
      den.swap(x.k2);
   }

   template <typename> friend struct spec_object_traits;
};

template <typename Coefficient, typename Exponent>
struct spec_object_traits< Serialized< RationalFunction<Coefficient, Exponent> > > :
   spec_object_traits<is_composite> {

   typedef RationalFunction<Coefficient, Exponent> masquerade_for;

   typedef cons<typename RationalFunction<Coefficient, Exponent>::term_hash,
           cons<typename RationalFunction<Coefficient, Exponent>::term_hash,
                typename RationalFunction<Coefficient, Exponent>::ring_type> > elements;

   template <typename Me, typename Visitor>
   static void visit_elements(Me& me, Visitor& v)
   {
      v << me.num.data->the_terms << me.den.data->the_terms << me.num.data->ring;
      set_den_ring(me.num, me.den);
   }

private:
   static void set_den_ring(const UniPolynomial<Coefficient, Exponent>&, const UniPolynomial<Coefficient, Exponent>&) {}

   static void set_den_ring(UniPolynomial<Coefficient, Exponent>& num, UniPolynomial<Coefficient, Exponent>& den)
   {
      den.data->ring=num.data->ring;
   }
};

template <typename Coefficient, typename Exponent>
struct choose_generic_object_traits< RationalFunction<Coefficient, Exponent>, false, false > :
      spec_object_traits< RationalFunction<Coefficient, Exponent> > {
   typedef void generic_type;
   typedef is_polynomial generic_tag;
   typedef RationalFunction<Coefficient, Exponent> persistent_type;

   static
   bool is_zero(const persistent_type& p)
   {
      return numerator(p).trivial();
   }

   static
   bool is_one(const persistent_type& p)
   {
      return numerator(p).unit() && denominator(p).unit();
   }

   static
   const persistent_type& zero()
   {
      static const persistent_type x=persistent_type();
      return x;
   }

   static
   const persistent_type& one()
   {
      static const persistent_type x(1);
      return x;
   }
};

template <typename Coefficient, typename Exponent>
struct algebraic_traits< RationalFunction<Coefficient, Exponent> > {
   typedef RationalFunction<typename algebraic_traits<Coefficient>::field_type, Exponent> field_type;
};

template <typename Coefficient, typename Exponent, typename T, typename TModel>
struct isomorphic_types_impl<RationalFunction<Coefficient, Exponent>, T,
                             typename enable_if<is_polynomial, (fits_as_coefficient<T, UniPolynomial<Coefficient, Exponent> >::value ||
                                                                is_unipolynomial_type<T, Coefficient, Exponent>::value)>::type,
                             TModel>
   : False {
   typedef cons<is_polynomial, is_scalar> discriminant;
};

template <typename Coefficient, typename Exponent, typename T, typename TModel>
struct isomorphic_types_impl<T, RationalFunction<Coefficient, Exponent>, TModel,
                             typename enable_if<is_polynomial, (fits_as_coefficient<T, UniPolynomial<Coefficient, Exponent> >::value ||
                                                                is_unipolynomial_type<T, Coefficient, Exponent>::value)>::type>
   : False {
   typedef cons<is_scalar, is_polynomial> discriminant;
};

template <typename Coefficient, typename Exponent>
struct isomorphic_types_impl<RationalFunction<Coefficient, Exponent>, RationalFunction<Coefficient, Exponent>, is_polynomial, is_polynomial>
   : True {
   typedef cons<is_polynomial, is_polynomial> discriminant;
};


template <typename Coefficient, typename Exponent, typename T> inline
typename enable_if<RationalFunction<Coefficient, Exponent>,
                   is_unipolynomial_type<T, Coefficient, Exponent>::value>::type
operator/ (const UniPolynomial<Coefficient, Exponent>& p1, const T& p2)
{
   return RationalFunction<Coefficient, Exponent>(p1, p2);
}

template <typename Coefficient, typename Exponent, typename T> inline
typename enable_if<RationalFunction<Coefficient, Exponent>,
                   is_unipolynomial_type<T, Coefficient, Exponent>::value>::type
operator/ (const UniTerm<Coefficient, Exponent>& p1, const T& p2)
{
   return RationalFunction<Coefficient, Exponent>(p1, p2);
}

template <typename Coefficient, typename Exponent, typename T> inline
typename enable_if<RationalFunction<Coefficient, Exponent>,
                   is_unipolynomial_type<T, Coefficient, Exponent>::value>::type
operator/ (const UniMonomial<Coefficient, Exponent>& p1, const T& p2)
{
   return RationalFunction<Coefficient, Exponent>(p1, p2);
}

template <typename Coefficient, typename Exponent, typename T> inline
typename enable_if<RationalFunction<Coefficient, Exponent>,
                   fits_as_coefficient<T, UniPolynomial<Coefficient, Exponent> >::value>::type
operator/ (const T& p1, const UniPolynomial<Coefficient, Exponent>& p2)
{
   return RationalFunction<Coefficient, Exponent>(p1, p2);
}

template <typename Coefficient, typename Exponent, typename T> inline
typename enable_if<RationalFunction<Coefficient, Exponent>,
                   fits_as_coefficient<T, UniTerm<Coefficient, Exponent> >::value>::type
operator/ (const T& p1, const UniTerm<Coefficient, Exponent>& p2)
{
   return RationalFunction<Coefficient, Exponent>(p1, p2);
}

template <typename Coefficient, typename Exponent, typename T> inline
typename enable_if<RationalFunction<Coefficient, Exponent>,
                   fits_as_coefficient<T, UniMonomial<Coefficient, Exponent> >::value>::type
operator/ (const T& p1, const UniMonomial<Coefficient, Exponent>& p2)
{
   return RationalFunction<Coefficient, Exponent>(p1, p2);
}

namespace operations {

// operations neg, add, sub, mul defined in Polynomial.h will instantiate correctly for RationalFunction too.

template <typename LeftRef, class RightRef>
struct div_impl<LeftRef, RightRef,
                typename enable_if< cons<is_polynomial, is_polynomial>, is_instance2_of<typename deref<LeftRef>::type, RationalFunction>::value>::type> {
   typedef LeftRef first_argument_type;
   typedef RightRef second_argument_type;
   typedef typename deref<LeftRef>::type result_type;

   result_type operator() (typename function_argument<LeftRef>::const_type l,
                           typename function_argument<RightRef>::const_type r) const
   {
      return l/r;
   }

   void assign(typename lvalue_arg<LeftRef>::type l, typename function_argument<RightRef>::const_type r) const
   {
      l/=r;
   }
};

template <typename LeftRef, class RightRef>
struct div_impl<LeftRef, RightRef,
                typename enable_if< cons<is_scalar, is_polynomial>, is_instance2_of<typename deref<RightRef>::type, RationalFunction>::value>::type> {
   typedef LeftRef first_argument_type;
   typedef RightRef second_argument_type;
   typedef typename deref<RightRef>::type result_type;

   result_type operator() (typename function_argument<LeftRef>::const_type l,
                           typename function_argument<RightRef>::const_type r) const
   {
      return l/r;
   }
};

template <typename LeftRef, class RightRef>
struct div_impl<LeftRef, RightRef,
                typename enable_if< cons<is_polynomial, is_scalar>, is_instance2_of<typename deref<LeftRef>::type, RationalFunction>::value>::type> {
   typedef LeftRef first_argument_type;
   typedef RightRef second_argument_type;
   typedef typename deref<LeftRef>::type result_type;

   result_type operator() (typename function_argument<LeftRef>::const_type l,
                           typename function_argument<RightRef>::const_type r) const
   {
      return l/r;
   }

   void assign(typename lvalue_arg<LeftRef>::type l, typename function_argument<RightRef>::const_type r) const
   {
      l/=r;
   }
};

} // end namespace operations 


template<typename Coefficient, typename Exponent>
class conv<UniPolynomial<Coefficient, Exponent>, RationalFunction<Coefficient, Exponent> > {
public:
   typedef UniPolynomial<Coefficient, Exponent> argument_type;
   typedef RationalFunction<Coefficient, Exponent> result_type;
   result_type operator() (const UniPolynomial<Coefficient, Exponent>& p) const { return p.to_rational_function(); }
};

template<typename Coefficient, typename Exponent>
class conv<RationalFunction<Coefficient, Exponent>, UniPolynomial<Coefficient, Exponent> > {
public:
   typedef RationalFunction<Coefficient, Exponent> argument_type;
   typedef UniPolynomial<Coefficient, Exponent> result_type;
   result_type operator() (const RationalFunction<Coefficient, Exponent>& f) const { return f.to_unipolynomial(); }
};


} // end namespace pm

namespace polymake {
   using pm::RationalFunction;
}

namespace std {
   template <typename Coefficient, typename Exponent>
   void swap(pm::RationalFunction<Coefficient,Exponent>& x1, pm::RationalFunction<Coefficient,Exponent>& x2) { x1.swap(x2); }
}

#endif // POLYMAKE_RATIONAL_FUNCTION_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
