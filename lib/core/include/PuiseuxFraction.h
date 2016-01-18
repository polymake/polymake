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

#ifndef POLYMAKE_VALUATED_RATIONAL_FUNCTION_H
#define POLYMAKE_VALUATED_RATIONAL_FUNCTION_H

#include "polymake/Rational.h"
#include "polymake/RationalFunction.h"
#include "polymake/TropicalNumber.h"
#include "polymake/linalg.h"


namespace pm {

struct is_valuated_rational_function {};

namespace operations {

template <typename OpRef, typename T>
struct evaluate;

template <typename OpRef>
struct evaluate<OpRef,double>;

}

template <typename MinMax, typename Coefficient = Rational, typename Exponent = Rational>
class PuiseuxFraction;

template <typename MinMax, typename Coefficient, typename Exponent>
struct matching_ring< PuiseuxFraction<MinMax, Coefficient, Exponent> > : True
{
   typedef Ring<Coefficient, Exponent> type;
};



template <typename MinMax, typename Coefficient, typename Exponent>
class PuiseuxFraction {
protected:
   RationalFunction<Coefficient,Exponent> rf;

public:
   typedef RationalFunction<Coefficient,Exponent> rf_type;
   typedef typename RationalFunction<Coefficient,Exponent>::polynomial_type polynomial_type;
   typedef Exponent exponent_type;
   typedef Coefficient coefficient_type;

   typedef typename polynomial_type::term_hash term_hash;
   typedef typename polynomial_type::ring_type ring_type;
   typedef typename polynomial_type::term_type term_type;
   typedef typename polynomial_type::monomial_type monomial_type;


   template <typename> friend struct spec_object_traits;
   template <typename,bool,bool> friend struct choose_generic_object_traits;
   template <typename> friend class std::numeric_limits;

   /// Construct a zero value.
   PuiseuxFraction() : rf() {}

   /// Construct a zero value of explicitely given ring type.
   explicit PuiseuxFraction(const typename rf_type::ring_type& r) : rf(r) {}

   /// One argument which may be a coefficient-compatible type or a unipolynomial
   template <typename T>
   explicit PuiseuxFraction(const T& t, typename enable_if<void**,
            (fits_as_coefficient<T, polynomial_type>::value || is_unipolynomial_type<T, Coefficient, Exponent>::value)
         >::type=0) : rf(t) {}

   /// One argument which may be a coefficient-compatible type or a unipolynomial and a ring
   template <typename T>
   PuiseuxFraction(const T& t, const typename rf_type::ring_type& r, typename enable_if<void**,
            (fits_as_coefficient<T, polynomial_type>::value || is_unipolynomial_type<T, Coefficient, Exponent>::value)
         >::type=0) : rf(t) {}
   PuiseuxFraction(const RationalFunction<Coefficient,Exponent>& t) : rf(numerator(t),denominator(t)) {}

   /// Two arguments which may be coefficient-compatible type or unipolynomial
   template <typename T1, typename T2>
   PuiseuxFraction(const T1& t1, const T2& t2, typename enable_if<void**, (
               (fits_as_coefficient<T1, polynomial_type>::value || is_unipolynomial_type<T1, Coefficient, Exponent>::value)
               && (fits_as_coefficient<T2, polynomial_type>::value || is_unipolynomial_type<T2, Coefficient, Exponent>::value)
            )>::type=0) : rf(t1, t2) {}

   /// Two arguments which may be coefficient-compatible type or unipolynomial and a ring
   template <typename T1, typename T2>
   PuiseuxFraction(const T1& t1, const T2& t2, const typename rf_type::ring_type& r, typename enable_if<void**, (
               (fits_as_coefficient<T1, polynomial_type>::value || is_unipolynomial_type<T1, Coefficient, Exponent>::value)
               && (fits_as_coefficient<T2, polynomial_type>::value || is_unipolynomial_type<T2, Coefficient, Exponent>::value)
            )>::type=0) : rf(t1, t2, r) {}

#if POLYMAKE_DEBUG
   ~PuiseuxFraction() { POLYMAKE_DEBUG_METHOD(PuiseuxFraction,dump); }
   void dump() const { rf.dump(); }
#endif

   template <typename T>
   typename enable_if<PuiseuxFraction&, (fits_as_coefficient<T, polynomial_type>::value || is_unipolynomial_type<T, Coefficient, Exponent>::value)>::type
   operator=(const T& t)
   {
      this->rf = RationalFunction<Coefficient,Exponent>(t);
      return *this;
   }

   const int orientation() {
      return MinMax::orientation();
   }

   const typename rf_type::ring_type& get_ring() const { return rf.get_ring(); }

   friend
   const polynomial_type& numerator(const PuiseuxFraction& me) { return numerator(me.rf); }

   friend
   const polynomial_type& denominator(const PuiseuxFraction& me) { return denominator(me.rf); }

   void swap(PuiseuxFraction& other)
   {
      rf.swap(other);
   }

   PuiseuxFraction& negate()
   {
      rf.negate();
      return *this;
   }

   /// PLUS
   PuiseuxFraction operator- () const
   {
      return PuiseuxFraction(-rf);
   }

   template <typename T>
   typename enable_if<PuiseuxFraction&, (fits_as_coefficient<T, polynomial_type>::value || is_unipolynomial_type<T, Coefficient, Exponent>::value)>::type
   operator+=(const T& t)
   {
      rf+=t;
      return *this;
   }

   PuiseuxFraction& operator+=(const PuiseuxFraction& other)
   {
      rf+=other.rf;
      return *this;
   }

   template <typename T>
   typename enable_if<PuiseuxFraction, (fits_as_coefficient<T, polynomial_type>::value || is_unipolynomial_type<T, Coefficient, Exponent>::value)>::type friend
   operator+(const PuiseuxFraction& vrf, const T& t)
   {
      return PuiseuxFraction(vrf.rf+t);
   }

   template <typename T>
   typename enable_if<PuiseuxFraction, (fits_as_coefficient<T, polynomial_type>::value || is_unipolynomial_type<T, Coefficient, Exponent>::value)>::type friend
   operator+(const T& t, const PuiseuxFraction& vrf)
   {
      return PuiseuxFraction(vrf.rf+t);
   }

   friend
   PuiseuxFraction operator+(const PuiseuxFraction& vrf1, const PuiseuxFraction& vrf2)
   {
      return PuiseuxFraction(vrf1.rf+vrf2.rf);
   }


   /// MINUS
   template <typename T>
   typename enable_if<PuiseuxFraction&, (fits_as_coefficient<T, polynomial_type>::value || is_unipolynomial_type<T, Coefficient, Exponent>::value)>::type
   operator-=(const T& t)
   {
      rf-=t;
      return *this;
   }

   PuiseuxFraction& operator-=(const PuiseuxFraction& other)
   {
      rf-=other.rf;
      return *this;
   }

   template <typename T>
   typename enable_if<PuiseuxFraction, (fits_as_coefficient<T, polynomial_type>::value || is_unipolynomial_type<T, Coefficient, Exponent>::value)>::type friend
   operator-(const PuiseuxFraction& vrf, const T& t)
   {
      return PuiseuxFraction(vrf.rf-t);
   }

   template <typename T>
   typename enable_if<PuiseuxFraction, (fits_as_coefficient<T, polynomial_type>::value || is_unipolynomial_type<T, Coefficient, Exponent>::value)>::type friend
   operator-(const T& t, const PuiseuxFraction& vrf)
   {
      return PuiseuxFraction(t-vrf.rf);
   }

   friend
   PuiseuxFraction operator-(const PuiseuxFraction& vrf1, const PuiseuxFraction& vrf2)
   {
      return PuiseuxFraction(vrf1.rf-vrf2.rf);
   }


   /// MULTIPLICATION
   template <typename T>
   typename enable_if<PuiseuxFraction&, (fits_as_coefficient<T, polynomial_type>::value || is_unipolynomial_type<T, Coefficient, Exponent>::value)>::type
   operator*=(const T& t)
   {
      rf*=t;
      return *this;
   }

   PuiseuxFraction& operator*=(const PuiseuxFraction& other)
   {
      rf*=other.rf;
      return *this;
   }

   template <typename T>
   typename enable_if<PuiseuxFraction, (fits_as_coefficient<T, polynomial_type>::value || is_unipolynomial_type<T, Coefficient, Exponent>::value)>::type friend
   operator*(const PuiseuxFraction& vrf, const T& t)
   {
      return PuiseuxFraction(vrf.rf*t);
   }

   template <typename T>
   typename enable_if<PuiseuxFraction, (fits_as_coefficient<T, polynomial_type>::value || is_unipolynomial_type<T, Coefficient, Exponent>::value)>::type friend
   operator*(const T& t, const PuiseuxFraction& vrf)
   {
      return PuiseuxFraction(vrf.rf*t);
   }

   friend
   PuiseuxFraction operator*(const PuiseuxFraction& vrf1, const PuiseuxFraction& vrf2)
   {
      return PuiseuxFraction(vrf1.rf*vrf2.rf);
   }


   /// DIVISION
   template <typename T>
   typename enable_if<PuiseuxFraction&, (fits_as_coefficient<T, polynomial_type>::value || is_unipolynomial_type<T, Coefficient, Exponent>::value)>::type
   operator/=(const T& t)
   {
      rf/=t;
      return *this;
   }

   PuiseuxFraction& operator/=(const PuiseuxFraction& other)
   {
      rf/=other.rf;
      return *this;
   }

   template <typename T>
   typename enable_if<PuiseuxFraction, (fits_as_coefficient<T, polynomial_type>::value || is_unipolynomial_type<T, Coefficient, Exponent>::value)>::type friend
   operator/(const PuiseuxFraction& vrf, const T& t)
   {
      return PuiseuxFraction(vrf.rf/t);
   }

   template <typename T>
   typename enable_if<PuiseuxFraction, (fits_as_coefficient<T, polynomial_type>::value || is_unipolynomial_type<T, Coefficient, Exponent>::value)>::type friend
   operator/(const T& t, const PuiseuxFraction& vrf)
   {
      return PuiseuxFraction(t/vrf.rf);
   }

   friend
   PuiseuxFraction operator/(const PuiseuxFraction& vrf1, const PuiseuxFraction& vrf2)
   {
      return PuiseuxFraction(vrf1.rf/vrf2.rf);
   }


   /// EQUALITY
   template <typename T>
   typename enable_if<bool, (fits_as_coefficient<T, polynomial_type>::value || is_unipolynomial_type<T, Coefficient, Exponent>::value)>::type friend
   operator==(const PuiseuxFraction& vrf, const T& t)
   {
      return vrf.rf == t;
   }

   template <typename T>
   typename enable_if<bool, (fits_as_coefficient<T, polynomial_type>::value || is_unipolynomial_type<T, Coefficient, Exponent>::value)>::type friend
   operator==(const T& t, const PuiseuxFraction& vrf)
   {
      return vrf == t;
   }

   friend
   bool operator==(const PuiseuxFraction& vrf1, const PuiseuxFraction& vrf2)
   {
      return vrf1.rf == vrf2.rf;
   }

   template <typename T>
   typename enable_if<bool, (fits_as_coefficient<T, polynomial_type>::value || is_unipolynomial_type<T, Coefficient, Exponent>::value)>::type friend
   operator!=(const PuiseuxFraction& vrf, const T& t)
   {
      return !(vrf == t);
   }

   template <typename T>
   typename enable_if<bool, (fits_as_coefficient<T, polynomial_type>::value || is_unipolynomial_type<T, Coefficient, Exponent>::value)>::type friend
   operator!=(const T& t, const PuiseuxFraction& vrf)
   {
      return !(vrf == t);
   }

   friend
   bool operator!=(const PuiseuxFraction& vrf1, const PuiseuxFraction& vrf2)
   {
      return !(vrf1.rf == vrf2.rf);
   }


   /// COMPARISON depending on Min / Max
   // Max::orientation = -1 and we need the highest term
   // Min::orientation =  1 and we need the lowest term
   cmp_value compare(const PuiseuxFraction& vrf) const
   {
      if (pm::identical<Max,MinMax>::value)
         return operations::cmp()((
                     numerator(rf)*denominator(vrf) - numerator(vrf)*denominator(rf)
                  ).lc(), zero_value<Coefficient>());
      else
         return operations::cmp()(
               (sign(denominator(rf).lc(-1))*sign(denominator(vrf).lc(-1))
                  *(numerator(rf)*denominator(vrf) - numerator(vrf)*denominator(rf)).lc(-1)
               ), zero_value<Coefficient>());
   }

   template <typename T>
   typename enable_if<cmp_value, fits_as_coefficient<T, polynomial_type>::value>::type
   compare(const T& c) const
   {
      const bool is_max = pm::identical<Max,MinMax>::value;
      Coefficient val;
      if (is_max)
      {
         if (!numerator(rf).trivial() && (is_zero(c) || numerator(rf).deg() > denominator(rf).deg()))
            val = numerator(rf).lc();
         else if (numerator(rf).deg() < denominator(rf).deg())
            val = -c;
         else
            val = numerator(rf).lc()-c;
      }
      else
      {
         const Exponent minus_one = -one_value<Exponent>();
         if (!numerator(rf).trivial() && (is_zero(c) || numerator(rf).lower_deg() < denominator(rf).lower_deg()))
            val = numerator(rf).lc(minus_one)*sign(denominator(rf).lc(minus_one));
         else if (numerator(rf).lower_deg() > denominator(rf).lower_deg())
            val = -c*abs(denominator(rf).lc(minus_one));
         else
            val = numerator(rf).lc(minus_one)*sign(denominator(rf).lc(minus_one))-c*abs(denominator(rf).lc(minus_one));
      }
      return operations::cmp()(val,zero_value<Coefficient>());
   }

   template <typename T>
   typename enable_if<cmp_value, is_unipolynomial_type<T, Coefficient, Exponent>::value>::type
   compare(const T& c) const
   {
      return compare(PuiseuxFraction(c));
   }

   template <typename T> friend
   typename enable_if<bool, (fits_as_coefficient<T, polynomial_type>::value || is_unipolynomial_type<T, Coefficient, Exponent>::value)>::type
   operator< (const PuiseuxFraction& vrf, const T& c)
   {
      return vrf.compare(c) == cmp_lt;
   }

   template <typename T> friend
   typename enable_if<bool, (fits_as_coefficient<T, polynomial_type>::value || is_unipolynomial_type<T, Coefficient, Exponent>::value)>::type
   operator<= (const PuiseuxFraction& vrf, const T& c)
   {
      return vrf.compare(c) != cmp_gt;
   }

   template <typename T> friend
   typename enable_if<bool, (fits_as_coefficient<T, polynomial_type>::value || is_unipolynomial_type<T, Coefficient, Exponent>::value)>::type
   operator> (const PuiseuxFraction& vrf, const T& c)
   {
      return vrf.compare(c) == cmp_gt;
   }

   template <typename T> friend
   typename enable_if<bool, (fits_as_coefficient<T, polynomial_type>::value || is_unipolynomial_type<T, Coefficient, Exponent>::value)>::type
   operator>= (const PuiseuxFraction& vrf, const T& c)
   {
      return vrf.compare(c) != cmp_lt;
   }

   template <typename T> friend
   typename enable_if<bool, (fits_as_coefficient<T, polynomial_type>::value || is_unipolynomial_type<T, Coefficient, Exponent>::value)>::type
   operator< (const T& c, const PuiseuxFraction& vrf)
   {
      return vrf>c;
   }

   template <typename T> friend
   typename enable_if<bool, (fits_as_coefficient<T, polynomial_type>::value || is_unipolynomial_type<T, Coefficient, Exponent>::value)>::type
   operator<= (const T& c, const PuiseuxFraction& vrf)
   {
      return vrf>=c;
   }

   template <typename T> friend
   typename enable_if<bool, (fits_as_coefficient<T, polynomial_type>::value || is_unipolynomial_type<T, Coefficient, Exponent>::value)>::type
   operator> (const T& c, const PuiseuxFraction& vrf)
   {
      return vrf<c;
   }

   template <typename T> friend
   typename enable_if<bool, (fits_as_coefficient<T, polynomial_type>::value || is_unipolynomial_type<T, Coefficient, Exponent>::value)>::type
   operator>= (const T& c, const PuiseuxFraction& vrf)
   {
      return vrf<=c;
   }



   friend
   bool operator< (const PuiseuxFraction& vrf1, const PuiseuxFraction& vrf2)
   {
      return vrf1.compare(vrf2) == cmp_lt;
   }

   friend
   bool operator<= (const PuiseuxFraction& vrf1, const PuiseuxFraction& vrf2)
   {
      return vrf1.compare(vrf2) != cmp_gt;
   }

   friend
   bool operator> (const PuiseuxFraction& vrf1, const PuiseuxFraction& vrf2)
   {
      return vrf1.compare(vrf2) == cmp_gt;
   }

   friend
   bool operator>= (const PuiseuxFraction& vrf1, const PuiseuxFraction& vrf2)
   {
      return vrf1.compare(vrf2) != cmp_lt;
   }

   template <typename Output> friend
   Output& operator<< (GenericOutput<Output>& out, const PuiseuxFraction& vrf)
   {
      out.top() << '(';
      numerator(vrf).pretty_print(out,cmp_monomial_ordered<Exponent>(-1 * MinMax::orientation()));
      out.top() << ')';
      if (!is_one(denominator(vrf))) {
         out.top() << "/(";
         denominator(vrf).pretty_print(out,cmp_monomial_ordered<Exponent>(-1 * MinMax::orientation()));
         out.top() << ')';
      }
      return out.top();
   }

   RationalFunction<Coefficient, Exponent> to_rationalfunction() const {
      return rf;
   }

private:
   // helper functions to define the valuation
   template <typename MinMaxT>
   typename enable_if<TropicalNumber<MinMaxT,Exponent>, pm::identical<Max,MinMaxT>::value>::type
   val() const
   {
      return TropicalNumber<MinMaxT,Exponent>(numerator(rf).deg() - denominator(rf).deg());
   }

   template <typename MinMaxT>
   typename enable_if<TropicalNumber<MinMaxT,Exponent>, pm::identical<Min,MinMaxT>::value>::type
   val() const
   {
      return TropicalNumber<MinMaxT,Exponent>(numerator(rf).lower_deg() - denominator(rf).lower_deg());
   }

public:
   TropicalNumber<MinMax,Exponent> val() const
   {
      return this->template val<MinMax>();
   }

   friend PuiseuxFraction abs(const PuiseuxFraction& vrf)
   {
      return vrf >= 0 ? vrf : -vrf;
   }

   friend bool abs_equal(const PuiseuxFraction& vrf1, const PuiseuxFraction& vrf2)
   {
      return abs(vrf1).compare(abs(vrf2)) == cmp_eq;
   }


   // this evaluates at t^exp_lcm and exp_lcm must be large enough such that this makes all needed
   // exponents integral
   template <typename T>
   typename enable_if<typename algebraic_traits<T>::field_type, fits_as_coefficient<T, polynomial_type>::value>::type
   evaluate_exp(const T& t, const long exp_lcm=1) const
   {
      typedef typename algebraic_traits<T>::field_type field;
      field val = numerator(*this).evaluate(t,exp_lcm);
      val /= denominator(*this).evaluate(t,exp_lcm);
      return val;
   }

   double evaluate_float(const double arg) const
   {
      double val = numerator(*this).evaluate_float(arg);
      val /= denominator(*this).evaluate_float(arg);
      return val;
   }


   template <typename T>
   typename enable_if<typename algebraic_traits<T>::field_type, (is_field_of_fractions<Exponent>::value && fits_as_coefficient<T, polynomial_type>::value)>::type
   evaluate(const T& t, const long exp=1) const
   {
      Integer exp_lcm(exp);
      exp_lcm = lcm(denominators(numerator(rf).monomials_as_vector() | denominator(rf).monomials_as_vector()));
      const double t_approx = std::pow(convert_to<double>(t),1.0/convert_to<double>(exp_lcm));
      Coefficient base = exp_lcm == exp ? t : t_approx;

      return this->evaluate_exp(base, exp_lcm.to_long());
   }

   template <typename T>
   typename enable_if<typename algebraic_traits<T>::field_type, (std::numeric_limits<Exponent>::is_integer && fits_as_coefficient<T, polynomial_type>::value)>::type
   evaluate(const T& t, const long exp=1) const
   {
      return this->evaluate_exp(t, exp);
   }


   template <typename VectorType, typename T> static
   typename enable_if<
         const LazyVector1<const VectorType&, operations::evaluate<PuiseuxFraction, typename algebraic_traits<T>::field_type> >,
         (fits_as_coefficient<T, polynomial_type>::value && is_field_of_fractions<Exponent>::value)
      >::type
   evaluate(const GenericVector<VectorType,PuiseuxFraction>& vec, const T& t, const long exp=1)
   {
      Integer exp_lcm(exp);
      for (typename Entire<VectorType>::const_iterator v = entire(vec.top()); !v.at_end(); ++v)
         exp_lcm = lcm(denominators(numerator(*v).monomials_as_vector() | denominator(*v).monomials_as_vector()) | exp_lcm);

      const double t_approx = std::pow(convert_to<double>(t),1.0/convert_to<double>(exp_lcm));
      const typename algebraic_traits<T>::field_type base = exp_lcm == exp ? t : t_approx;

      return LazyVector1<
               const VectorType&,
               operations::evaluate<PuiseuxFraction, typename algebraic_traits<T>::field_type>
             >(vec.top(),operations::evaluate<PuiseuxFraction, typename algebraic_traits<T>::field_type>(base,exp_lcm.to_long()));
   }

   template <typename VectorType, typename T> static
   typename enable_if<
         const LazyVector1<const VectorType&, operations::evaluate<PuiseuxFraction,T> >,
         (fits_as_coefficient<T, polynomial_type>::value && std::numeric_limits<Exponent>::is_integer)
      >::type
   evaluate(const GenericVector<VectorType,PuiseuxFraction>& vec, const T& t, const long exp=1)
   {
      return LazyVector1<const VectorType&, operations::evaluate<PuiseuxFraction,T> >(vec.top(),operations::evaluate<PuiseuxFraction,T>(t,exp));
   }

   template <typename MatrixType, typename T> static
   typename enable_if<
         const LazyMatrix1<const MatrixType&, operations::evaluate<PuiseuxFraction, typename algebraic_traits<T>::field_type> >,
         (fits_as_coefficient<T, polynomial_type>::value && is_field_of_fractions<Exponent>::value)
      >::type
   evaluate(const GenericMatrix<MatrixType,PuiseuxFraction>& m, const T& t, const long exp=1)
   {
      Integer exp_lcm(exp);
      for (typename Entire<ConcatRows<MatrixType> >::const_iterator e = entire(concat_rows(m.top())); !e.at_end(); ++e)
         exp_lcm = lcm(denominators(numerator(*e).monomials_as_vector() | denominator(*e).monomials_as_vector()) | exp_lcm);

      const double t_approx = std::pow(convert_to<double>(t),1.0/convert_to<double>(exp_lcm));
      const typename algebraic_traits<T>::field_type base = exp_lcm == exp ? t : t_approx;

      return LazyMatrix1<
               const MatrixType&,
               operations::evaluate<PuiseuxFraction, typename algebraic_traits<T>::field_type>
             >(m.top(), operations::evaluate<PuiseuxFraction, typename algebraic_traits<T>::field_type>(base,exp_lcm.to_long()));
   }

   template <typename MatrixType, typename T> static
   typename enable_if<
         const LazyMatrix1<const MatrixType&, operations::evaluate<PuiseuxFraction,T> >,
         (fits_as_coefficient<T, polynomial_type>::value && std::numeric_limits<Exponent>::is_integer)
      >::type
   evaluate(const GenericMatrix<MatrixType,PuiseuxFraction>& m, const T& t, const long exp=1)
   {
      return LazyMatrix1<
               const MatrixType&,
               operations::evaluate<PuiseuxFraction,T>
             >(m.top(),operations::evaluate<PuiseuxFraction,T>(t,exp));
   }



   template <typename VectorType> static
   const LazyVector1<const VectorType&, operations::evaluate<PuiseuxFraction,double> >
   evaluate_float(const GenericVector<VectorType,PuiseuxFraction>& vec, const double t)
   {
      return LazyVector1<const VectorType&, operations::evaluate<PuiseuxFraction,double> >(vec.top(),operations::evaluate<PuiseuxFraction,double>(t));
   }

   template <typename MatrixType> static
   const LazyMatrix1<const MatrixType&, operations::evaluate<PuiseuxFraction,double> >
   evaluate_float(const GenericMatrix<MatrixType,PuiseuxFraction>& vec, const double t)
   {
      return LazyMatrix1<const MatrixType&, operations::evaluate<PuiseuxFraction,double> >(vec.top(),operations::evaluate<PuiseuxFraction,double>(t));
   }

};


template <typename OpRef, typename T>
struct operations::evaluate {
   typedef OpRef argument_type;
   typedef typename algebraic_traits<T>::field_type result_type;

   evaluate(const T& t, const long e=1) : val(t), exp(e) { }

   result_type operator() (typename function_argument<OpRef>::const_type x) const
   {
      return x.evaluate_exp(val,exp);
   }

private:
   const T val;
   const long exp;
};

template <typename OpRef>
struct operations::evaluate<OpRef,double> {
   typedef OpRef argument_type;
   typedef double result_type;

   evaluate(const double& t, const int e=1) : val(t), exp(e) { }

   result_type operator() (typename function_argument<OpRef>::const_type x) const
   {
      return x.evaluate_float(std::pow(val,exp));
   }

private:
   const double val;
   const int exp;
};



template <typename MinMax, typename Coefficient, typename Exponent>
struct algebraic_traits< PuiseuxFraction<MinMax,Coefficient,Exponent> >{
   typedef PuiseuxFraction<MinMax,typename algebraic_traits<Coefficient>::field_type,Exponent> field_type;
};


template <typename MinMax, typename Coefficient, typename Exponent>
struct spec_object_traits< Serialized< PuiseuxFraction<MinMax, Coefficient, Exponent> > > :
   spec_object_traits<is_composite> {

   typedef PuiseuxFraction<MinMax, Coefficient, Exponent> masquerade_for;

   typedef RationalFunction<Coefficient,Exponent> elements;

   template <typename Me, typename Visitor>
   static void visit_elements(Me& me, Visitor& v)
   {
      v << me.rf;
   }
};


template <typename MinMax, typename Coefficient, typename Exponent>
struct choose_generic_object_traits< PuiseuxFraction<MinMax,Coefficient,Exponent>, false, false > :
   spec_object_traits<PuiseuxFraction<MinMax,Coefficient,Exponent> > {
   typedef PuiseuxFraction<MinMax, Coefficient, Exponent> persistent_type;
   typedef void generic_type;
   typedef is_scalar generic_tag;


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

template <typename MinMax, typename Coefficient, typename Exponent, typename Scalar>
class conv<PuiseuxFraction<MinMax,Coefficient, Exponent>, Scalar> {
public:
   typedef PuiseuxFraction<MinMax,Coefficient, Exponent> argument_type;
   typedef Scalar result_type;
   result_type operator() (const argument_type& f) const
   {
      if (convertible_to<Coefficient, Scalar>::value &&
            denominator(f).unit() &&
            numerator(f).deg() == 0 && numerator(f).lower_deg() == 0)
         return conv<Coefficient,Scalar>()(numerator(f).lc());
      else
         throw std::runtime_error("Conversion to scalar not possible."); }
};

template <typename MinMax, typename Coefficient, typename Exponent>
class conv<PuiseuxFraction<MinMax,Coefficient, Exponent>, TropicalNumber<MinMax,Exponent> > {
public:
   typedef PuiseuxFraction<MinMax,Coefficient, Exponent> argument_type;
   typedef TropicalNumber<MinMax,Exponent> result_type;
   result_type operator() (const argument_type& f) const { return f.val(); }
};

// to make a nice error if someone tries to convert to the wrong tropical number
template <typename MinMax, typename Coefficient, typename Exponent>
class conv<PuiseuxFraction<MinMax,Coefficient, Exponent>, TropicalNumber<typename MinMax::dual,Exponent> > {
public:
   typedef PuiseuxFraction<MinMax,Coefficient, Exponent> argument_type;
   typedef TropicalNumber<MinMax,Exponent> result_type;
   result_type operator() (const argument_type& f) const { throw std::runtime_error("incompatible types Min / Max"); }
};

template <typename MinMax, typename Coefficient, typename Exponent>
class conv<Coefficient, PuiseuxFraction<MinMax,Coefficient,Exponent> > : public conv_by_cast<Coefficient, PuiseuxFraction<MinMax,Coefficient,Exponent> > {};

template <typename Scalar, typename MinMax, typename Coefficient, typename Exponent>
class conv<Scalar, PuiseuxFraction<MinMax,Coefficient, Exponent> > : public if_else< (convertible_to<Scalar, Coefficient>::value ||
                                                                  explicitly_convertible_to<Scalar, Coefficient>::value),
                                                                 conv_by_cast<Scalar, PuiseuxFraction<MinMax,Coefficient,Exponent> >,
                                                                 nothing >::type {};

// disambiguation
template <typename MinMax, typename Coefficient, typename Exponent>
class conv<PuiseuxFraction<MinMax,Coefficient,Exponent>, PuiseuxFraction<MinMax,Coefficient,Exponent> > : public trivial_conv< PuiseuxFraction<MinMax,Coefficient,Exponent> > {};

template <typename MinMax, typename Coefficient, typename Exponent>
class conv<PuiseuxFraction<MinMax,Coefficient,Exponent>, PuiseuxFraction<typename MinMax::dual,Coefficient,Exponent> > {
public:
   typedef PuiseuxFraction<MinMax,Coefficient, Exponent> argument_type;
   typedef PuiseuxFraction<typename MinMax::dual,Coefficient, Exponent> result_type;
   result_type operator() (const argument_type& f) const { throw std::runtime_error("incompatible types Min / Max"); }
};


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

#endif // POLYMAKE_VALUATED_RATIONAL_FUNCTION_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
