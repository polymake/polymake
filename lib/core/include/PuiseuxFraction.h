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

#pragma once

#include <cstddef>
#include "polymake/Rational.h"
#include "polymake/RationalFunction.h"
#include "polymake/TropicalNumber.h"
#include "polymake/linalg.h"


namespace pm {

namespace operations {

template <typename OpRef, typename T>
struct evaluate;

template <typename OpRef>
struct evaluate<OpRef,double>;

}

template <typename MinMax, typename Coefficient, typename Exponent>
class PuiseuxFraction;
template <typename MinMax, typename Coefficient, typename Exponent>
class PuiseuxFraction_generic;
template <typename MinMax>
class PuiseuxFraction_subst;

namespace pf_internal {

template <typename MinMax, typename Coefficient, typename Exponent>
struct impl_chooser {
   using type = PuiseuxFraction_generic<MinMax, Coefficient, Exponent>;
};

template <typename MinMax>
struct impl_chooser<MinMax, Rational, Rational> {
   using type = PuiseuxFraction_subst<MinMax>;
};

template <typename T>
auto exp_to_int(const T& t, Int& exp_den,
                std::enable_if_t<is_unipolynomial_type<T, Rational, Rational>::value, std::nullptr_t> = nullptr)
{
   auto exps = t.monomials_as_vector();
   exp_den = static_cast<Int>(lcm(denominators(exps)|exp_den));
   return UniPolynomial<Rational, Int>(t.coefficients_as_vector(), convert_to<Int>(exps * exp_den));
}

template <typename T>
auto exp_to_int(const T& t1, const T& t2, Int& exp_den,
                std::enable_if_t<is_unipolynomial_type<T, Rational, Rational>::value, std::nullptr_t> = nullptr)
{
   auto exps1 = t1.monomials_as_vector();
   auto exps2 = t2.monomials_as_vector();
   exp_den = static_cast<Int>(lcm(denominators(exps1) | denominators(exps2) | exp_den));
   return RationalFunction<Rational, Int>(
            UniPolynomial<Rational, Int>(t1.coefficients_as_vector(), convert_to<Int>(exps1 * exp_den)),
            UniPolynomial<Rational, Int>(t2.coefficients_as_vector(), convert_to<Int>(exps2 * exp_den))
          );
}

template <typename T>
auto exp_to_int(const T& t, Int& exp_den,
                std::enable_if_t<UniPolynomial<Rational,Rational>::fits_as_coefficient<T>::value, std::nullptr_t> = nullptr)
{
   return UniPolynomial<Rational, Int>(t);
}

inline
auto exp_to_int(const RationalFunction<Rational, Rational>& t, Int& exp_den)
{
   return exp_to_int(numerator(t), denominator(t), exp_den);
}

}


template <typename MinMax, typename Coefficient, typename Exponent>
class PuiseuxFraction_base {
public:
   typedef RationalFunction<Coefficient, Exponent> rf_type;

   typedef typename rf_type::polynomial_type polynomial_type;
   typedef Exponent exponent_type;
   typedef Coefficient coefficient_type;

   template <typename> friend struct spec_object_traits;
   template <typename,bool,bool> friend struct choose_generic_object_traits;
   template <typename> friend class std::numeric_limits;

   template <typename T>
   using fits_as_coefficient = typename rf_type::template fits_as_coefficient<T>;

   template <typename T>
   using fits_as_particle = typename rf_type::template fits_as_particle<T>;

   template <typename T>
   struct is_compatible
      : bool_constant<fits_as_particle<T>::value || std::is_same<T, PuiseuxFraction<MinMax, Coefficient, Exponent>>::value> {};

   template <typename T>
   struct is_comparable
      : bool_constant<fits_as_particle<T>::value || std::is_same<T, TropicalNumber<MinMax, Exponent>>::value> {};

   template <typename T>
   struct is_comparable_or_same
      : bool_constant<is_comparable<T>::value || std::is_same<T, PuiseuxFraction<MinMax,Coefficient,Exponent>>::value> {};

   const int orientation() const
   {
      return MinMax::orientation();
   }


};

template <typename MinMax, typename Coefficient, typename Exponent>
class PuiseuxFraction_generic : public PuiseuxFraction_base<MinMax,Coefficient,Exponent> {
protected:
   using base = PuiseuxFraction_base<MinMax,Coefficient,Exponent>;

public:
   using rf_type = typename base::rf_type;
   using polynomial_type = typename base::polynomial_type;
   template <typename T>
   using fits_as_particle = typename base::template fits_as_particle<T>;
   template <typename T>
   using fits_as_coefficient = typename base::template fits_as_coefficient<T>;
   template <typename T>
   using is_compatible = typename base::template is_compatible<T>;
   template <typename T>
   using is_comparable = typename base::template is_comparable<T>;
   template <typename T>
   using is_comparable_or_same = typename base::template is_comparable_or_same<T>;

protected:
   rf_type rf;

public:
   template <typename> friend struct spec_object_traits;
   template <typename,bool,bool> friend struct choose_generic_object_traits;
   template <typename> friend class std::numeric_limits;

   /// Construct a zero value.
   PuiseuxFraction_generic() : rf() {}

   /// One argument which may be a coefficient-compatible type or a unipolynomial
   template <typename T,
             typename=std::enable_if_t<fits_as_particle<T>::value>>
   explicit PuiseuxFraction_generic(const T& t)
      : rf(t) {}

   PuiseuxFraction_generic(const rf_type& t)
      : rf(numerator(t),denominator(t)) {}

   /// Two arguments which may be coefficient-compatible type or unipolynomial
   template <typename T1, typename T2,
             typename=std::enable_if_t<fits_as_particle<T1>::value && fits_as_particle<T2>::value>>
   PuiseuxFraction_generic(const T1& t1, const T2& t2)
      : rf(t1, t2) {}

   template <typename T,
             typename=std::enable_if_t<fits_as_particle<T>::value>>
   PuiseuxFraction_generic& operator= (const T& t)
   {
      rf = rf_type(t);
      return *this;
   }

   PuiseuxFraction_generic& operator= (const PuiseuxFraction_generic& pf)
   {
      rf = pf.rf;
      return *this;
   }

   friend
   const polynomial_type& numerator(const PuiseuxFraction_generic& me) { return numerator(me.rf); }

   friend
   const polynomial_type& denominator(const PuiseuxFraction_generic& me) { return denominator(me.rf); }

   bool is_zero() const
   {
      return pm::is_zero(rf);
   }

   bool is_one() const
   {
      return pm::is_one(rf);
   }

   void swap(PuiseuxFraction_generic& other)
   {
      rf.swap(other);
   }

   PuiseuxFraction_generic& negate()
   {
      rf.negate();
      return *this;
   }

   friend
   PuiseuxFraction_generic operator- (const PuiseuxFraction_generic& me)
   {
      return PuiseuxFraction_generic(-me.rf);
   }

   /// PLUS

   template <typename T,
             typename=std::enable_if_t<fits_as_particle<T>::value>>
   PuiseuxFraction_generic& operator+= (const T& r)
   {
      rf += r;
      return *this;
   }

   PuiseuxFraction_generic& operator+= (const PuiseuxFraction_generic& r)
   {
      rf += r.rf;
      return *this;
   }


   /// MINUS

   template <typename T,
             typename=std::enable_if_t<fits_as_particle<T>::value>>
   PuiseuxFraction_generic& operator-= (const T& r)
   {
      rf -= r;
      return *this;
   }

   PuiseuxFraction_generic& operator-= (const PuiseuxFraction_generic& r)
   {
      rf -= r.rf;
      return *this;
   }



   /// MULTIPLICATION
   template <typename T,
             typename=std::enable_if_t<fits_as_particle<T>::value>>
   PuiseuxFraction_generic& operator*= (const T& r)
   {
      rf *= r;
      return *this;
   }

   PuiseuxFraction_generic& operator*= (const PuiseuxFraction_generic& r)
   {
      rf *= r.rf;
      return *this;
   }

   /// DIVISION
   template <typename T,
             typename=std::enable_if_t<fits_as_particle<T>::value>>
   PuiseuxFraction_generic& operator/= (const T& r)
   {
      rf /= r;
      return *this;
   }

   PuiseuxFraction_generic& operator/= (const PuiseuxFraction_generic& r)
   {
      rf /= r.rf;
      return *this;
   }

   /// EQUALITY
   bool operator== (const PuiseuxFraction_generic& r) const
   {
      return rf == r.rf;
   }

   template <typename T,
             typename=std::enable_if_t<fits_as_particle<T>::value>>
   bool operator== (const T& r) const
   {
      return rf == r;
   }

   template <typename T,
             typename=std::enable_if_t<is_comparable<T>::value>>
   friend
   bool operator== (const T& l, const PuiseuxFraction_generic& r)
   {
      return r==l;
   }


   const rf_type& to_rationalfunction() const
   {
      return rf;
   }

   template <typename Exp=Exponent, typename T>
   PuiseuxFraction_generic<MinMax,Coefficient,Exp> substitute_monomial(const T& t) const
   {
      return PuiseuxFraction_generic<MinMax,Coefficient,Exp>(rf.template substitute_monomial<Exp>(t));
   }

   size_t get_hash() const noexcept { return rf.get_hash(); }

#if POLYMAKE_DEBUG
   void dump() const __attribute__((used)) { rf.dump(); }
#endif

   template <typename,typename,typename>
   friend class PuiseuxFraction;
};



template <typename MinMax>
class PuiseuxFraction_subst : public PuiseuxFraction_base<MinMax,Rational,Rational> {
public:
   using pf_type = PuiseuxFraction_generic<MinMax, Rational, Rational>;
   using rf_type = typename pf_type::rf_type;
   using polynomial_type = typename pf_type::polynomial_type;
   using Coefficient = Rational;
   using Exponent = Rational;
protected:
   using base = PuiseuxFraction_base<MinMax,Rational,Rational>;
   using subst_exponent = Int;
   using subst_pf_type = PuiseuxFraction<MinMax, Coefficient, subst_exponent>;
   using subst_rf_type = typename subst_pf_type::rf_type;
   using subst_poly_type = UniPolynomial<Coefficient, subst_exponent>;

public:
   template <typename T>
   using fits_as_coefficient = typename base::template fits_as_coefficient<T>;
   template <typename T>
   using fits_as_particle = typename base::template fits_as_particle<T>;
   template <typename T>
   using is_compatible = typename base::template is_compatible<T>;
   template <typename T>
   using is_comparable = typename base::template is_comparable<T>;
   template <typename T>
   using is_comparable_or_same = typename base::template is_comparable_or_same<T>;
   template <typename T>
   using is_subst_polynomial = is_unipolynomial_type<T,Coefficient,subst_exponent>;
   template <typename T>
   using is_base_polynomial = is_unipolynomial_type<T,Coefficient,Exponent>;

protected:

   // the exp_den must be the first member
   Int exp_den = 1;
   subst_pf_type subst_pf;

   mutable std::unique_ptr<rf_type> rf_cache;

   template <typename T, std::enable_if_t<is_subst_polynomial<T>::value, std::nullptr_t> = nullptr>
   explicit PuiseuxFraction_subst(const T& t)
      : exp_den(1)
      , subst_pf(t)
   { }

   template <typename T, std::enable_if_t<is_subst_polynomial<T>::value, std::nullptr_t> = nullptr>
   PuiseuxFraction_subst(const T& num, const T& den)
      : exp_den(1)
      , subst_pf(num,den)
   { }

   template <typename T, std::enable_if_t<is_among<T, subst_rf_type, subst_pf_type>::value, std::nullptr_t> = nullptr>
   explicit PuiseuxFraction_subst(const T& rf)
      : exp_den(1)
      , subst_pf(rf)
   { }

   void normalize_den()
   {
      if (exp_den == 1)
         return;
      auto exps1 = numerator(subst_pf).monomials_as_vector();
      auto exps2 = denominator(subst_pf).monomials_as_vector();
      Int expgcd = gcd(exps1 | exps2 | exp_den);
      if (expgcd == 1)
         return;
      subst_pf = subst_pf.template substitute_monomial<Int>(Rational(1, expgcd));
      exp_den /= expgcd;
   }

public:

   /// Construct a zero value.
   PuiseuxFraction_subst()
      : exp_den(1)
      , subst_pf() {}

   /// copy
   PuiseuxFraction_subst(const PuiseuxFraction_subst& pf)
      : exp_den(pf.exp_den)
      , subst_pf(pf.subst_pf) {}

   PuiseuxFraction_subst(const subst_pf_type& pf, Int expden)
      : exp_den(expden)
      , subst_pf(pf) {}

   /// construct from coefficient or unipolynomial
   template <typename T,
             std::enable_if_t<fits_as_particle<T>::value, std::nullptr_t> = nullptr>
   explicit PuiseuxFraction_subst(const T& t)
      : exp_den(1)
      , subst_pf(pf_internal::exp_to_int(t, exp_den)) {}

   /// construct from two coefficient or unipolynomial
   template <typename T1, typename T2,
             std::enable_if_t<fits_as_particle<T1>::value && fits_as_particle<T2>::value, std::nullptr_t> = nullptr>
   PuiseuxFraction_subst(const T1& t1, const T2& t2)
      : exp_den(1)
      , subst_pf(pf_internal::exp_to_int(polynomial_type(t1), polynomial_type(t2), exp_den)) {}

   /// construct from rational function
   explicit PuiseuxFraction_subst(const rf_type& t)
      : exp_den(1)
      , subst_pf(pf_internal::exp_to_int(t, exp_den)) {}

   template <typename T,
             std::enable_if_t<fits_as_particle<T>::value || std::is_same<T,rf_type>::value, std::nullptr_t> = nullptr>
   PuiseuxFraction_subst& operator= (const T& t)
   {
      exp_den = 1;
      subst_pf = subst_pf_type(pf_internal::exp_to_int(t, exp_den));
      rf_cache.reset(nullptr);
      return *this;
   }

   PuiseuxFraction_subst& operator= (const PuiseuxFraction_subst& pf)
   {
      exp_den = pf.exp_den;
      subst_pf = pf.subst_pf;
      rf_cache.reset(nullptr);
      return *this;
   }

   friend
   const polynomial_type& numerator(const PuiseuxFraction_subst& me)
   {
      return numerator(me.to_rationalfunction());
   }

   friend
   const polynomial_type& denominator(const PuiseuxFraction_subst& me)
   {
      return denominator(me.to_rationalfunction());
   }

   bool is_zero() const
   {
      return pm::is_zero(subst_pf);
   }

   bool is_one() const
   {
      return pm::is_one(subst_pf);
   }

   const rf_type& to_rationalfunction() const
   {
      if (!rf_cache)
         rf_cache.reset(new rf_type(numerator(subst_pf).template substitute_monomial<Rational>(Rational(1,exp_den)),
                                    denominator(subst_pf).template substitute_monomial<Rational>(Rational(1,exp_den))));
      return *rf_cache;
   }

   void swap(PuiseuxFraction_subst& other)
   {
      std::swap(exp_den,other.exp_den);
      subst_pf.swap(other);
      std::swap(rf_cache,other.rf_cache);
   }

   PuiseuxFraction_subst& negate()
   {
      subst_pf.negate();
      rf_cache.reset(nullptr);
      return *this;
   }

   friend
   PuiseuxFraction_subst operator- (const PuiseuxFraction_subst& me)
   {
      return PuiseuxFraction_subst(-me.subst_pf,me.exp_den);
   }


   /// PLUS
   template <typename T>
   std::enable_if_t<fits_as_coefficient<T>::value, PuiseuxFraction_subst&>
   operator+= (const T& r)
   {
      subst_pf += r;
      normalize_den();
      rf_cache.reset(nullptr);
      return *this;
   }

   template <typename T>
   std::enable_if_t<is_base_polynomial<T>::value, PuiseuxFraction_subst&>
   operator+= (const T& r)
   {
      const Int old_den = exp_den;
      auto poly_int = pf_internal::exp_to_int(r, exp_den);
      if (old_den != exp_den)
         subst_pf = subst_pf.template substitute_monomial<Int>(exp_den/old_den);
      subst_pf += poly_int;
      normalize_den();
      rf_cache.reset(nullptr);
      return *this;
   }

   PuiseuxFraction_subst& operator+= (const PuiseuxFraction_subst& r)
   {
      const Int new_den = lcm(exp_den, r.exp_den);
      if (new_den != exp_den)
         subst_pf = subst_pf.template substitute_monomial<Int>(new_den/exp_den);
      if (new_den != r.exp_den) {
         subst_pf += r.subst_pf.template substitute_monomial<Int>(new_den/r.exp_den);
      } else {
         subst_pf += r.subst_pf;
      }
      exp_den = new_den;
      normalize_den();
      rf_cache.reset(nullptr);
      return *this;
   }

   /// MINUS
   template <typename T,
             typename = std::enable_if_t<fits_as_particle<T>::value || std::is_same<T, PuiseuxFraction_subst>::value>>
   PuiseuxFraction_subst& operator-= (const T& r)
   {
      subst_pf += -r;
      normalize_den();
      rf_cache.reset(nullptr);
      return *this;
   }

   /// MULTIPLICATION
   template <typename T>
   std::enable_if_t<fits_as_coefficient<T>::value, PuiseuxFraction_subst&>
   operator*= (const T& r)
   {
      subst_pf *= r;
      if (pm::is_zero(r))
         exp_den = 1;
      rf_cache.reset(nullptr);
      return *this;
   }

   template <typename T>
   std::enable_if_t<is_base_polynomial<T>::value, PuiseuxFraction_subst&>
   operator*= (const T& r)
   {
      const Int old_den = exp_den;
      auto poly_int = pf_internal::exp_to_int(r, exp_den);
      if (old_den != exp_den)
         subst_pf = subst_pf.template substitute_monomial<Int>(exp_den/old_den);
      subst_pf *= poly_int;
      normalize_den();
      rf_cache.reset(nullptr);
      return *this;
   }

   PuiseuxFraction_subst& operator*= (const PuiseuxFraction_subst& r)
   {
      const Int new_den = lcm(exp_den, r.exp_den);
      if (new_den != exp_den)
         subst_pf = subst_pf.template substitute_monomial<Int>(new_den / exp_den);
      if (new_den != r.exp_den) {
         subst_pf *= r.subst_pf.template substitute_monomial<Int>(new_den / r.exp_den);
      } else {
         subst_pf *= r.subst_pf;
      }
      exp_den = new_den;
      normalize_den();
      rf_cache.reset(nullptr);
      return *this;
   }

   /// DIVISION
   template <typename T>
   std::enable_if_t<fits_as_coefficient<T>::value, PuiseuxFraction_subst&>
   operator/= (const T& r)
   {
      subst_pf /= r;
      rf_cache.reset(nullptr);
      return *this;
   }

   template <typename T>
   std::enable_if_t<is_base_polynomial<T>::value, PuiseuxFraction_subst&>
   operator/= (const T& r)
   {
      const Int old_den = exp_den;
      auto poly_int = pf_internal::exp_to_int(r, exp_den);
      if (old_den != exp_den)
         subst_pf = subst_pf.template substitute_monomial<Int>(exp_den / old_den);
      subst_pf /= poly_int;
      normalize_den();
      rf_cache.reset(nullptr);
      return *this;
   }

   PuiseuxFraction_subst& operator/= (const PuiseuxFraction_subst& r)
   {
      const Int new_den = lcm(exp_den, r.exp_den);
      if (new_den != exp_den)
         subst_pf = subst_pf.template substitute_monomial<Int>(new_den / exp_den);
      if (new_den != r.exp_den) {
         subst_pf /= r.subst_pf.template substitute_monomial<Int>(new_den / r.exp_den);
      } else {
         subst_pf /= r.subst_pf;
      }
      exp_den = new_den;
      normalize_den();
      rf_cache.reset(nullptr);
      return *this;
   }

   /// EQUALITY
   bool operator== (const PuiseuxFraction_subst& r) const
   {
      return exp_den == r.exp_den && subst_pf == r.subst_pf;
   }

   template <typename T>
   std::enable_if_t<fits_as_coefficient<T>::value, bool>
   operator== (const T& r) const
   {
      return subst_pf == r;
   }

   template <typename T>
   std::enable_if_t<is_base_polynomial<T>::value, bool>
   operator== (const T& r) const
   {
      const Int den = exp_den;
      return subst_pf == pf_internal::exp_to_int(r, den) && exp_den == den;
   }

   template <typename Exp = Exponent, typename T>
   auto substitute_monomial(const T& t, std::enable_if_t<!std::is_same<Exp,Rational>::value, std::nullptr_t> = nullptr) const
   {
      return PuiseuxFraction_generic<MinMax,Coefficient,Exp>(subst_pf.template substitute_monomial<Exp>(t/exp_den));
   }

   template <typename Exp = Exponent,typename T>
   auto substitute_monomial(const T& t, std::enable_if_t<std::is_same<Exp,Rational>::value, std::nullptr_t> = nullptr) const
   {
      Rational exp(t);
      exp /= exp_den;
      return PuiseuxFraction_subst(subst_pf.template substitute_monomial<Int>(numerator(exp)),denominator(exp));
   }


   size_t get_hash() const noexcept { size_t h = exp_den; hash_combine(h,subst_pf.get_hash()); return h; }

#if POLYMAKE_DEBUG
   void dump() const __attribute__((used))
   {
      std::cerr << "exp_den: " << exp_den << std::endl;
      subst_pf.dump();
   }
#endif

   template <typename,typename,typename>
   friend class PuiseuxFraction;
};


template <typename MinMax, typename Coefficient=Rational, typename Exponent=Rational>
class PuiseuxFraction : public pf_internal::impl_chooser<MinMax, Coefficient, Exponent>::type {
protected:
   using impl_type = typename pf_internal::impl_chooser<MinMax, Coefficient, Exponent>::type;
   using base = PuiseuxFraction_base<MinMax, Coefficient, Exponent>;

public:
   using rf_type = typename base::rf_type;
   using polynomial_type = typename base::polynomial_type;
   template <typename T>
   using fits_as_coefficient = typename base::template fits_as_coefficient<T>;
   template <typename T>
   using fits_as_particle = typename base::template fits_as_particle<T>;
   template <typename T>
   using is_compatible = typename base::template is_compatible<T>;
   template <typename T>
   using is_comparable = typename base::template is_comparable<T>;
   template <typename T>
   using is_comparable_or_same = typename base::template is_comparable_or_same<T>;

   // constructors
   using impl_type::impl_type;

   PuiseuxFraction() : impl_type() {}

   // upgrade from impl
   PuiseuxFraction(const impl_type& pf) : impl_type(pf) {}


   template <typename T,
             typename=std::enable_if_t<fits_as_particle<T>::value || std::is_same<T,rf_type>::value>>
   PuiseuxFraction& operator= (const T& t)
   {
      static_cast<impl_type&>(*this) = t;
      return *this;
   }

   // forwards to impl type

   friend
   PuiseuxFraction operator- (const PuiseuxFraction& me)
   {
      return impl_type(me).negate();
   }

   PuiseuxFraction& negate()
   {
      impl_type::negate();
      return *this;
   }

   // PLUS
   template <typename T,
             typename=std::enable_if_t<is_compatible<T>::value>>
   PuiseuxFraction& operator+= (const T& r)
   {
      impl_type::operator+=(r);
      return *this;
   }

   template <typename T,
             typename=std::enable_if_t<fits_as_particle<T>::value>>
   friend
   PuiseuxFraction operator+ (const PuiseuxFraction& l, const T& r)
   {
      return impl_type(l) += r;
   }

   template <typename T,
             typename=std::enable_if_t<fits_as_particle<T>::value>>
   friend
   PuiseuxFraction operator+ (const T& l, const PuiseuxFraction& r)
   {
      return impl_type(l) += r;
   }

   friend
   PuiseuxFraction operator+ (const PuiseuxFraction& l, const PuiseuxFraction& r)
   {
      return impl_type(l) += impl_type(r);
   }

   // MINUS
   template <typename T,
             typename=std::enable_if_t<is_compatible<T>::value>>
   PuiseuxFraction& operator-= (const T& r)
   {
      impl_type::operator+=(-r);
      return *this;
   }

   template <typename T,
             typename=std::enable_if_t<fits_as_particle<T>::value>>
   friend
   PuiseuxFraction operator- (const PuiseuxFraction& l, const T& r)
   {
      return impl_type(l) += -r;
   }

   template <typename T,
             typename=std::enable_if_t<fits_as_particle<T>::value>>
   friend
   PuiseuxFraction operator- (const T& l, const PuiseuxFraction& r)
   {
      return impl_type(l) += -r;
   }

   friend
   PuiseuxFraction operator- (const PuiseuxFraction& l, const PuiseuxFraction& r)
   {
      return impl_type(l) += -r;
   }


   // MULTIPLICATION
   template <typename T,
             typename=std::enable_if_t<is_compatible<T>::value>>
   PuiseuxFraction& operator*= (const T& r)
   {
      impl_type::operator*=(r);
      return *this;
   }

   template <typename T,
             typename=std::enable_if_t<fits_as_particle<T>::value>>
   friend
   PuiseuxFraction operator* (const PuiseuxFraction& l, const T& r)
   {
      return impl_type(l) *= r;
   }

   template <typename T,
             typename=std::enable_if_t<fits_as_particle<T>::value>>
   friend
   PuiseuxFraction operator* (const T& l, const PuiseuxFraction& r)
   {
      return impl_type(l) *= r;
   }

   friend
   PuiseuxFraction operator* (const PuiseuxFraction& l, const PuiseuxFraction& r)
   {
      return impl_type(l) *= r;
   }


   // DIVISION
   template <typename T,
             typename=std::enable_if_t<is_compatible<T>::value>>
   PuiseuxFraction& operator/= (const T& r)
   {
      impl_type::operator/=(r);
      return *this;
   }

   template <typename T,
             typename=std::enable_if_t<fits_as_particle<T>::value>>
   friend
   PuiseuxFraction operator/ (const PuiseuxFraction& l, const T& r)
   {
      return impl_type(l) /= r;
   }

   template <typename T,
             typename=std::enable_if_t<fits_as_particle<T>::value>>
   friend
   PuiseuxFraction operator/ (const T& l, const PuiseuxFraction& r)
   {
      return impl_type(l) /= r;
   }

   friend
   PuiseuxFraction operator/ (const PuiseuxFraction& l, const PuiseuxFraction& r)
   {
      return impl_type(l) /= r;
   }

   // this evaluates at t^exp_lcm and exp_lcm must be large enough such that this makes all needed
   // exponents integral
   template <typename T>
   friend
   typename algebraic_traits<T>::field_type
   evaluate_exp(const PuiseuxFraction& me, const T& t, const Int exp_lcm = 1,
                std::enable_if_t<fits_as_coefficient<T>::value, std::nullptr_t> = nullptr)
   {
      using field = typename algebraic_traits<T>::field_type;
      field val = numerator(me).evaluate(t, exp_lcm);
      val /= denominator(me).evaluate(t, exp_lcm);
      return val;
   }

   template <typename VectorType, typename T>
   friend
   auto evaluate(VectorType&& vec, const T& t, const Int exp = 1,
                 std::enable_if_t<(fits_as_coefficient<T>::value &&
                                   is_field_of_fractions<Exponent>::value &&
                                   is_generic_vector<VectorType, PuiseuxFraction>::value), std::nullptr_t> = nullptr)
   {
      Integer exp_lcm(exp);
      for (auto v = entire(vec.top()); !v.at_end(); ++v)
         exp_lcm = lcm(denominators(numerator(*v).monomials_as_vector() | denominator(*v).monomials_as_vector()) | exp_lcm);

      const double t_approx = std::pow(double(t), 1.0/double(exp_lcm));
      using base_t = typename algebraic_traits<T>::field_type;
      const base_t baseval = exp_lcm == exp ? base_t{t} : base_t{t_approx};

      return LazyVector1<add_const_t<unwary_t<VectorType>>,
                         operations::evaluate<PuiseuxFraction, base_t>>
                        (unwary(std::forward<VectorType>(vec)), operations::evaluate<PuiseuxFraction, base_t>(baseval, Int(exp_lcm)));
   }

   template <typename VectorType, typename T>
   friend
   auto evaluate(VectorType&& vec, const T& t, const Int exp = 1,
                 std::enable_if_t<(fits_as_coefficient<T>::value &&
                                   std::numeric_limits<Exponent>::is_integer &&
                                   is_generic_vector<VectorType, PuiseuxFraction>::value), std::nullptr_t> = nullptr)
   {
      return LazyVector1<add_const_t<unwary_t<VectorType>>, operations::evaluate<PuiseuxFraction, T>>
                        (unwary(std::forward<VectorType>(vec)), operations::evaluate<PuiseuxFraction, T>(t, exp));
   }

   template <typename MatrixType, typename T>
   friend
   auto evaluate(MatrixType&& m, const T& t, const Int exp = 1,
                 std::enable_if_t<(fits_as_coefficient<T>::value &&
                                   is_field_of_fractions<Exponent>::value &&
                                   is_generic_matrix<MatrixType, PuiseuxFraction>::value), std::nullptr_t> = nullptr)
   {
      Integer exp_lcm(exp);
      for (auto e = entire(concat_rows(m.top())); !e.at_end(); ++e)
         exp_lcm = lcm(denominators(numerator(*e).monomials_as_vector() | denominator(*e).monomials_as_vector()) | exp_lcm);

      const double t_approx = std::pow(convert_to<double>(t),1.0/convert_to<double>(exp_lcm));
      using base_t = typename algebraic_traits<T>::field_type;
      const base_t baseval = exp_lcm == exp ? base_t{t} : base_t{t_approx};

      return LazyMatrix1<add_const_t<unwary_t<MatrixType>>,
                         operations::evaluate<PuiseuxFraction, base_t>>
                        (unwary(std::forward<MatrixType>(m)), operations::evaluate<PuiseuxFraction, base_t>(baseval, Int(exp_lcm)));
   }

   template <typename MatrixType, typename T>
   friend   
   auto evaluate(MatrixType&& m, const T& t, const Int exp = 1,
                 std::enable_if_t<(fits_as_coefficient<T>::value &&
                                   std::numeric_limits<Exponent>::is_integer &&
                                   is_generic_matrix<MatrixType, PuiseuxFraction>::value), std::nullptr_t> = nullptr)
   {
      return LazyMatrix1<add_const_t<unwary_t<MatrixType>>, operations::evaluate<PuiseuxFraction, T>>
                        (unwary(std::forward<MatrixType>(m)), operations::evaluate<PuiseuxFraction, T>(t, exp));
   }

   template <typename T>
   friend
   auto evaluate(const PuiseuxFraction& me, const T& t, const Int exp = 1,
                 std::enable_if_t<is_field_of_fractions<Exponent>::value && fits_as_coefficient<T>::value, std::nullptr_t> = nullptr)
   {
      Integer exp_lcm(exp);
      exp_lcm = lcm(denominators(numerator(me).monomials_as_vector() | denominator(me).monomials_as_vector()) | exp_lcm);
      const double t_approx = std::pow(convert_to<double>(t), 1.0 / convert_to<double>(exp_lcm));
      const Coefficient base_coeff = exp_lcm == exp ? Coefficient{t} : Coefficient{t_approx};
      return evaluate_exp(me, base_coeff, Int(exp_lcm));
   }

   template <typename T>
   friend
   auto evaluate(const PuiseuxFraction& me, const T& t, const Int exp = 1,
                 std::enable_if_t<std::numeric_limits<Exponent>::is_integer && fits_as_coefficient<T>::value, std::nullptr_t> = nullptr)
   {
      return evaluate_exp(me, t, exp);
   }

   template <typename VectorType>
   friend
   auto evaluate_float(VectorType&& vec, const double t,
                       std::enable_if_t<is_generic_vector<VectorType, PuiseuxFraction>::value, std::nullptr_t> = nullptr)
   {
      return LazyVector1<add_const_t<unwary_t<VectorType>>, operations::evaluate<PuiseuxFraction, double>>
                        (unwary(std::forward<VectorType>(vec)), operations::evaluate<PuiseuxFraction, double>(t));
   }

   template <typename MatrixType>
   friend
   auto evaluate_float(MatrixType&& m, const double t,
                       std::enable_if_t<is_generic_matrix<MatrixType, PuiseuxFraction>::value, std::nullptr_t> = nullptr)
   {
      return LazyMatrix1<add_const_t<unwary_t<MatrixType>>, operations::evaluate<PuiseuxFraction, double>>
                        (unwary(std::forward<MatrixType>(m)), operations::evaluate<PuiseuxFraction, double>(t));
   }

   friend
   double evaluate_float(const PuiseuxFraction& me, const double arg)
   {
      double val = numerator(me).evaluate_float(arg);
      val /= denominator(me).evaluate_float(arg);
      return val;
   }

   template <typename Exp = Exponent, typename T>
   PuiseuxFraction<MinMax, Coefficient, Exp> substitute_monomial(const T& t) const
   {
      return impl_type::template substitute_monomial<Exp>(t);
   }

   template <typename T,
             typename=std::enable_if_t<is_comparable_or_same<T>::value>>
   friend
   bool operator< (const PuiseuxFraction& l, const T& r)
   {
      return l.compare(r) < 0;
   }

   template <typename T,
             typename=std::enable_if_t<is_comparable<T>::value>>
   friend
   bool operator< (const T& l, const PuiseuxFraction& r)
   {
      return r.compare(l) > 0;
   }

   template <typename T,
             typename=std::enable_if_t<is_comparable_or_same<T>::value>>
   friend
   bool operator<= (const PuiseuxFraction& l, const T& r)
   {
      return l.compare(r) <= 0;
   }

   template <typename T,
             typename=std::enable_if_t<is_comparable<T>::value>>
   friend
   bool operator<= (const T& l, const PuiseuxFraction& r)
   {
      return r.compare(l) >= 0;
   }

   template <typename T,
             typename=std::enable_if_t<is_comparable_or_same<T>::value>>
   friend
   bool operator> (const PuiseuxFraction& l, const T& r)
   {
      return l.compare(r) > 0;
   }

   template <typename T,
             typename=std::enable_if_t<is_comparable<T>::value>>
   friend
   bool operator> (const T& l, const PuiseuxFraction& r)
   {
      return r.compare(l) < 0;
   }

   template <typename T,
             typename=std::enable_if_t<is_comparable_or_same<T>::value>>
   friend
   bool operator>= (const PuiseuxFraction& l, const T& r)
   {
      return l.compare(r) >= 0;
   }

   template <typename T,
             typename=std::enable_if_t<is_comparable<T>::value>>
   friend
   bool operator>= (const T& l, const PuiseuxFraction& r)
   {
      return r.compare(l) <= 0;
   }

   static
   const Array<std::string>& get_var_names()
   {
      return rf_type::get_var_names();
   }

   static
   void set_var_names(const Array<std::string>& names)
   {
      rf_type::set_var_names(names);
   }

   static
   void reset_var_names()
   {
      rf_type::reset_var_names();
   }

   /// COMPARISON depending on Min / Max
   // Max::orientation = -1 and we need the highest term
   // Min::orientation =  1 and we need the lowest term
   Int compare(const PuiseuxFraction& pf) const
   {
      if (std::is_same<Max, MinMax>::value)
         return sign((numerator(*this)*denominator(pf) - numerator(pf)*denominator(*this)).lc());
      else
         return sign(denominator(*this).lc(-1)) * sign(denominator(pf).lc(-1)) *
                sign((numerator(*this)*denominator(pf) - numerator(pf)*denominator(*this)).lc(-1));
   }

   template <typename T>
   std::enable_if_t<fits_as_coefficient<T>::value, Int>
   compare(const T& c) const
   {
      if (std::is_same<Max, MinMax>::value) {
         if (!numerator(*this).trivial() && (is_zero(c) || numerator(*this).deg() > denominator(*this).deg()))
            return sign(numerator(*this).lc());
         else if (numerator(*this).deg() < denominator(*this).deg())
            return -sign(c);
         else
            return sign(numerator(*this).lc()-c);
      } else {
         const Exponent minus_one = -one_value<Exponent>();
         if (!numerator(*this).trivial() && (is_zero(c) || numerator(*this).lower_deg() < denominator(*this).lower_deg()))
            return sign(numerator(*this).lc(minus_one)) * sign(denominator(*this).lc(minus_one));
         else if (numerator(*this).lower_deg() > denominator(*this).lower_deg())
            return -sign(c);
         else
            return sign(numerator(*this).lc(minus_one) * sign(denominator(*this).lc(minus_one)) - c*abs(denominator(*this).lc(minus_one)));
      }
   }


   template <typename T>
   std::enable_if_t<is_unipolynomial_type<T, Coefficient, Exponent>::value, Int>
   compare(const T& c) const
   {
      return compare(PuiseuxFraction(c));
   }

   Int compare(const TropicalNumber<MinMax, Exponent>& c) const
   {
      return val().compare(c);
   }

   template <typename T>
   std::enable_if_t<is_comparable_or_same<T>::value, bool>
   operator== (const T& r) const
   {
      return impl_type::operator==(r);
   }

   template <typename T>
   friend
   std::enable_if_t<is_comparable<T>::value, bool>
   operator== (const T& l, const PuiseuxFraction& r)
   {
      return r.operator==(l);
   }

   template <typename T>
   friend
   std::enable_if_t<is_comparable_or_same<T>::value, bool>
   operator!= (const PuiseuxFraction& l, const T& r)
   {
      return !(l == r);
   }

   template <typename T>
   friend
   std::enable_if_t<is_comparable<T>::value, bool>
   operator!= (const T& l, const PuiseuxFraction& r)
   {
      return !(r == l);
   }

   friend
   bool operator== (const PuiseuxFraction& l, const TropicalNumber<MinMax, Exponent>& r)
   {
      return l.val() == r;
   }

   static
   void swap_var_names(PolynomialVarNames& other_names)
   {
      rf_type::swap_var_names(other_names);
   }

   friend PuiseuxFraction abs(const PuiseuxFraction& pf)
   {
      return pf >= 0 ? pf : -pf;
   }

   friend bool abs_equal(const PuiseuxFraction& pf1, const PuiseuxFraction& pf2)
   {
      return abs(pf1).compare(abs(pf2)) == 0;
   }

   TropicalNumber<MinMax, Exponent> val() const
   {
      if (std::is_same<MinMax, Max>::value)
         return TropicalNumber<MinMax, Exponent>(numerator(*this).deg() - denominator(*this).deg());
      else
         return TropicalNumber<MinMax, Exponent>(numerator(*this).lower_deg() - denominator(*this).lower_deg());
   }

   explicit operator TropicalNumber<MinMax, Exponent> () const
   {
      return val();
   }

   template <typename Output, typename Order>
   void pretty_print(Output& out, const Order& order) const
   {
      out << '(';
      static_cast<polynomial_type>(numerator(*this)).print_ordered(out, order);
      out << ')';
      if (!denominator(*this).is_one()) {
         out << "/(";
         static_cast<polynomial_type>(denominator(*this)).print_ordered(out, order);
         out << ')';
      }
   }

   template <typename Output> friend
   Output& operator<< (GenericOutput<Output>& out, const PuiseuxFraction& pf)
   {
      pf.pretty_print(out.top(),-MinMax::orientation());
      return out.top();
   }

};



template <typename MinMax, typename Coefficient, typename Exponent>
Int sign(const PuiseuxFraction<MinMax, Coefficient, Exponent>& x)
{
   return x.compare(zero_value<Coefficient>());
}

namespace operations {

template <typename OpRef, typename T>
struct evaluate {
   typedef OpRef argument_type;
   typedef typename algebraic_traits<T>::field_type result_type;

   evaluate(const T& t, const long e = 1) : val(t), exp(e) { }

   result_type operator() (typename function_argument<OpRef>::const_type x) const
   {
      return evaluate_exp(x, val,exp);
   }

private:
   const T val;
   const long exp;
};

template <typename OpRef>
struct evaluate<OpRef, double> {
   typedef OpRef argument_type;
   typedef double result_type;

   evaluate(const double& t, const Int e = 1) : val(t), exp(e) { }

   result_type operator() (typename function_argument<OpRef>::const_type x) const
   {
      return evaluate_float(x, std::pow(val, exp));
   }

private:
   const double val;
   const Int exp;
};

}

template <typename MinMax, typename Coefficient, typename Exponent>
struct algebraic_traits< PuiseuxFraction<MinMax, Coefficient, Exponent> > {
   typedef PuiseuxFraction<MinMax,typename algebraic_traits<Coefficient>::field_type,Exponent> field_type;
};


template <typename MinMax, typename Coefficient, typename Exponent>
struct spec_object_traits< Serialized< PuiseuxFraction<MinMax, Coefficient, Exponent> > >
   : spec_object_traits<is_composite> {

   using masquerade_for = PuiseuxFraction<MinMax, Coefficient, Exponent>;
   using rf_type = typename masquerade_for::rf_type;
   using elements = rf_type;

   template <typename Visitor>
   static void visit_elements(Serialized<masquerade_for>& me, Visitor& v)
   {
      rf_type tmp;
      v << tmp;
      me = masquerade_for(tmp);
   }
   template <typename Visitor>
   static void visit_elements(const Serialized<masquerade_for>& me, Visitor& v)
   {
      v << me.to_rationalfunction();
   }
};


template <typename MinMax, typename Coefficient, typename Exponent>
struct choose_generic_object_traits< PuiseuxFraction<MinMax,Coefficient,Exponent>, false, false>
   : spec_object_traits<PuiseuxFraction<MinMax,Coefficient,Exponent> > {
   typedef PuiseuxFraction<MinMax, Coefficient, Exponent> persistent_type;
   typedef void generic_type;
   typedef is_scalar generic_tag;

   static
   bool is_zero(const persistent_type& p)
   {
      return p.is_zero();
   }

   static
   bool is_one(const persistent_type& p)
   {
      return p.is_one();
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

template <typename MinMax, typename Coefficient, typename Exponent>
struct hash_func<PuiseuxFraction<MinMax,Coefficient,Exponent>, is_scalar> {
   size_t operator() (const PuiseuxFraction<MinMax,Coefficient,Exponent>& p) const noexcept
   {
      return p.get_hash();
   }
};

namespace polynomial_impl {

template <typename MinMax, typename Coefficient, typename Exponent>
struct nesting_level< PuiseuxFraction<MinMax, Coefficient, Exponent> >
  : int_constant<nesting_level<Coefficient>::value+1> {};

}
} // end namespace pm

namespace polymake {
   using pm::PuiseuxFraction;
}

namespace std {
   template <typename MinMax, typename Coefficient, typename Exponent>
   void swap(pm::PuiseuxFraction<MinMax,Coefficient,Exponent>& x1, pm::PuiseuxFraction<MinMax,Coefficient,Exponent>& x2) { x1.swap(x2); }

   template <typename MinMax, typename Coefficient, typename Exponent>
   class numeric_limits<pm::PuiseuxFraction<MinMax,Coefficient,Exponent> > : public numeric_limits<Coefficient> {
   public:
      static const bool is_integer=false;
      static pm::PuiseuxFraction<MinMax,Coefficient,Exponent> min() throw() { return pm::PuiseuxFraction<MinMax,Coefficient,Exponent>(numeric_limits<Coefficient>::min()); }
      static pm::PuiseuxFraction<MinMax,Coefficient,Exponent> infinity() throw() { return pm::PuiseuxFraction<MinMax,Coefficient,Exponent>(numeric_limits<Coefficient>::infinity()); }
      static pm::PuiseuxFraction<MinMax,Coefficient,Exponent> max() throw() { return pm::PuiseuxFraction<MinMax,Coefficient,Exponent>(numeric_limits<Coefficient>::max()); }
   };
}


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
