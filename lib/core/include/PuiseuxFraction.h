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

template <typename MinMax, typename Coefficient=Rational, typename Exponent=Rational>
class PuiseuxFraction {
public:
   typedef RationalFunction<Coefficient, Exponent> rf_type;
protected:
   rf_type rf;
public:
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
   struct is_comparable
      : bool_constant<fits_as_particle<T>::value || std::is_same<T, TropicalNumber<MinMax, Exponent>>::value> {};

   template <typename T>
   struct is_comparable_or_same
      : bool_constant<is_comparable<T>::value || std::is_same<T, PuiseuxFraction>::value> {};

   /// Construct a zero value.
   PuiseuxFraction() : rf() {}

   /// One argument which may be a coefficient-compatible type or a unipolynomial
   template <typename T,
             typename=typename std::enable_if<fits_as_particle<T>::value>::type>
   explicit PuiseuxFraction(const T& t)
      : rf(t) {}

   PuiseuxFraction(const rf_type& t)
      : rf(numerator(t),denominator(t)) {}

   /// Two arguments which may be coefficient-compatible type or unipolynomial
   template <typename T1, typename T2,
             typename=typename std::enable_if<fits_as_particle<T1>::value && fits_as_particle<T2>::value>::type>
   PuiseuxFraction(const T1& t1, const T2& t2)
      : rf(t1, t2) {}

   template <typename T,
             typename=typename std::enable_if<fits_as_particle<T>::value>::type>
   PuiseuxFraction& operator= (const T& t)
   {
      rf = rf_type(t);
      return *this;
   }

   const int orientation()
   {
      return MinMax::orientation();
   }

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

   friend
   PuiseuxFraction operator- (const PuiseuxFraction& me)
   {
      return PuiseuxFraction(-me.rf);
   }

   /// PLUS

   template <typename T,
             typename=typename std::enable_if<fits_as_particle<T>::value>::type>
   PuiseuxFraction& operator+= (const T& r)
   {
      rf += r;
      return *this;
   }

   PuiseuxFraction& operator+= (const PuiseuxFraction& r)
   {
      rf += r.rf;
      return *this;
   }

   template <typename T,
             typename=typename std::enable_if<fits_as_particle<T>::value>::type>
   friend
   PuiseuxFraction operator+ (const PuiseuxFraction& l, const T& r)
   {
      return PuiseuxFraction(l.rf + r);
   }

   template <typename T,
             typename=typename std::enable_if<fits_as_particle<T>::value>::type>
   friend
   PuiseuxFraction operator+ (const T& l, const PuiseuxFraction& r)
   {
      return PuiseuxFraction(l + r.rf);
   }

   friend
   PuiseuxFraction operator+ (const PuiseuxFraction& l, const PuiseuxFraction& r)
   {
      return PuiseuxFraction(l.rf + r.rf);
   }


   /// MINUS
   template <typename T,
             typename=typename std::enable_if<fits_as_particle<T>::value>::type>
   PuiseuxFraction& operator-= (const T& r)
   {
      rf -= r;
      return *this;
   }

   PuiseuxFraction& operator-= (const PuiseuxFraction& r)
   {
      rf -= r.rf;
      return *this;
   }

   template <typename T,
             typename=typename std::enable_if<fits_as_particle<T>::value>::type>
   friend 
   PuiseuxFraction operator- (const PuiseuxFraction& l, const T& r)
   {
      return PuiseuxFraction(l.rf - r);
   }

   template <typename T,
             typename=typename std::enable_if<fits_as_particle<T>::value>::type>
   friend
   PuiseuxFraction operator- (const T& l, const PuiseuxFraction& r)
   {
      return PuiseuxFraction(l - r.rf);
   }

   friend
   PuiseuxFraction operator- (const PuiseuxFraction& l, const PuiseuxFraction& r)
   {
      return PuiseuxFraction(l.rf - r.rf);
   }


   /// MULTIPLICATION
   template <typename T,
             typename=typename std::enable_if<fits_as_particle<T>::value>::type>
   PuiseuxFraction& operator*= (const T& r)
   {
      rf *= r;
      return *this;
   }

   PuiseuxFraction& operator*= (const PuiseuxFraction& r)
   {
      rf *= r.rf;
      return *this;
   }

   template <typename T,
             typename=typename std::enable_if<fits_as_particle<T>::value>::type>
   friend
   PuiseuxFraction operator* (const PuiseuxFraction& l, const T& r)
   {
      return PuiseuxFraction(l.rf * r);
   }

   template <typename T,
             typename=typename std::enable_if<fits_as_particle<T>::value>::type>
   friend
   PuiseuxFraction operator* (const T& l, const PuiseuxFraction& r)
   {
      return PuiseuxFraction(l * r.rf);
   }

   friend
   PuiseuxFraction operator* (const PuiseuxFraction& l, const PuiseuxFraction& r)
   {
      return PuiseuxFraction(l.rf * r.rf);
   }


   /// DIVISION
   template <typename T,
             typename=typename std::enable_if<fits_as_particle<T>::value>::type>
   PuiseuxFraction& operator/= (const T& r)
   {
      rf /= r;
      return *this;
   }

   PuiseuxFraction& operator/= (const PuiseuxFraction& r)
   {
      rf /= r.rf;
      return *this;
   }

   template <typename T,
             typename=typename std::enable_if<fits_as_particle<T>::value>::type>
   friend
   PuiseuxFraction operator/ (const PuiseuxFraction& l, const T& r)
   {
      return PuiseuxFraction(l.rf / r);
   }

   template <typename T,
             typename=typename std::enable_if<fits_as_particle<T>::value>::type>
   friend
   PuiseuxFraction operator/ (const T& l, const PuiseuxFraction& r)
   {
      return PuiseuxFraction(l / r.rf);
   }

   friend
   PuiseuxFraction operator/ (const PuiseuxFraction& l, const PuiseuxFraction& r)
   {
      return PuiseuxFraction(l.rf / r.rf);
   }


   /// EQUALITY
   friend
   bool operator== (const PuiseuxFraction& l, const PuiseuxFraction& r)
   {
      return l.rf == r.rf;
   }

   template <typename T,
             typename=typename std::enable_if<fits_as_particle<T>::value>::type>
   friend
   bool operator== (const PuiseuxFraction& l, const T& r)
   {
      return l.rf == r;
   }

   friend
   bool operator== (const PuiseuxFraction& l, const TropicalNumber<MinMax, Exponent>& r)
   {
      return l.val() == r;
   }

   template <typename T,
             typename=typename std::enable_if<is_comparable<T>::value>::type>
   friend
   bool operator== (const T& l, const PuiseuxFraction& r)
   {
      return r==l;
   }

   template <typename T,
             typename=typename std::enable_if<is_comparable_or_same<T>::value>::type>
   friend
   bool operator!= (const PuiseuxFraction& l, const T& r)
   {
      return !(l == r);
   }

   template <typename T,
             typename=typename std::enable_if<is_comparable<T>::value>::type> friend
   bool operator!= (const T& l, const PuiseuxFraction& r)
   {
      return !(r == l);
   }


   /// COMPARISON depending on Min / Max
   // Max::orientation = -1 and we need the highest term
   // Min::orientation =  1 and we need the lowest term
   int compare(const PuiseuxFraction& vrf) const
   {
      if (std::is_same<Max, MinMax>::value)
         return sign((numerator(rf)*denominator(vrf) - numerator(vrf)*denominator(rf)).lc());
      else
         return sign(denominator(rf).lc(-1)) * sign(denominator(vrf).lc(-1)) *
                sign((numerator(rf)*denominator(vrf) - numerator(vrf)*denominator(rf)).lc(-1));
   }

   template <typename T>
   typename std::enable_if<fits_as_coefficient<T>::value, int>::type
   compare(const T& c) const
   {
      if (std::is_same<Max, MinMax>::value) {
         if (!numerator(rf).trivial() && (is_zero(c) || numerator(rf).deg() > denominator(rf).deg()))
            return sign(numerator(rf).lc());
         else if (numerator(rf).deg() < denominator(rf).deg())
            return -sign(c);
         else
            return sign(numerator(rf).lc()-c);
      } else {
         const Exponent minus_one = -one_value<Exponent>();
         if (!numerator(rf).trivial() && (is_zero(c) || numerator(rf).lower_deg() < denominator(rf).lower_deg()))
            return sign(numerator(rf).lc(minus_one)) * sign(denominator(rf).lc(minus_one));
         else if (numerator(rf).lower_deg() > denominator(rf).lower_deg())
            return -sign(c);
         else
            return sign(numerator(rf).lc(minus_one) * sign(denominator(rf).lc(minus_one)) - c*abs(denominator(rf).lc(minus_one)));
      }
   }

   template <typename T>
   typename std::enable_if<is_unipolynomial_type<T, Coefficient, Exponent>::value, int>::type
   compare(const T& c) const
   {
      return compare(PuiseuxFraction(c));
   }

   int compare(const TropicalNumber<MinMax, Exponent>& c) const
   {
      return val().compare(c);
   }

   template <typename T,
             typename=typename std::enable_if<is_comparable_or_same<T>::value>::type>
   friend
   bool operator< (const PuiseuxFraction& l, const T& r)
   {
      return l.compare(r) < 0;
   }

   template <typename T,
             typename=typename std::enable_if<is_comparable<T>::value>::type>
   friend
   bool operator< (const T& l, const PuiseuxFraction& r)
   {
      return r.compare(l) > 0;
   }

   template <typename T,
             typename=typename std::enable_if<is_comparable_or_same<T>::value>::type>
   friend
   bool operator<= (const PuiseuxFraction& l, const T& r)
   {
      return l.compare(r) <= 0;
   }

   template <typename T,
             typename=typename std::enable_if<is_comparable<T>::value>::type>
   friend
   bool operator<= (const T& l, const PuiseuxFraction& r)
   {
      return r.compare(l) >= 0;
   }

   template <typename T,
             typename=typename std::enable_if<is_comparable_or_same<T>::value>::type>
   friend
   bool operator> (const PuiseuxFraction& l, const T& r)
   {
      return l.compare(r) > 0;
   }

   template <typename T,
             typename=typename std::enable_if<is_comparable<T>::value>::type>
   friend
   bool operator> (const T& l, const PuiseuxFraction& r)
   {
      return r.compare(l) < 0;
   }

   template <typename T,
             typename=typename std::enable_if<is_comparable_or_same<T>::value>::type>
   friend
   bool operator>= (const PuiseuxFraction& l, const T& r)
   {
      return l.compare(r) >= 0;
   }

   template <typename T,
             typename=typename std::enable_if<is_comparable<T>::value>::type> friend
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

   static
   void swap_var_names(PolynomialVarNames& other_names)
   {
      rf_type::swap_var_names(other_names);
   }

   template <typename Output> friend
   Output& operator<< (GenericOutput<Output>& out, const PuiseuxFraction& vrf)
   {
      out.top() << '(';
      numerator(vrf).print_ordered(out, -MinMax::orientation());
      out.top() << ')';
      if (!is_one(denominator(vrf))) {
         out.top() << "/(";
         denominator(vrf).print_ordered(out, -MinMax::orientation());
         out.top() << ')';
      }
      return out.top();
   }

   rf_type to_rationalfunction() const
   {
      return rf;
   }

   TropicalNumber<MinMax, Exponent> val() const
   {
      if (is_zero(rf))
         return TropicalNumber<MinMax, Exponent>::zero();
      
      if (std::is_same<MinMax, Max>::value)
         return TropicalNumber<MinMax, Exponent>(numerator(rf).deg() - denominator(rf).deg());
      else
         return TropicalNumber<MinMax, Exponent>(numerator(rf).lower_deg() - denominator(rf).lower_deg());
   }

   friend PuiseuxFraction abs(const PuiseuxFraction& vrf)
   {
      return vrf >= 0 ? vrf : -vrf;
   }

   friend bool abs_equal(const PuiseuxFraction& vrf1, const PuiseuxFraction& vrf2)
   {
      return abs(vrf1).compare(abs(vrf2)) == 0;
   }


   // this evaluates at t^exp_lcm and exp_lcm must be large enough such that this makes all needed
   // exponents integral
   template <typename T,
             typename=typename std::enable_if<fits_as_coefficient<T>::value>::type>
   typename algebraic_traits<T>::field_type
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
   typename std::enable_if<is_field_of_fractions<Exponent>::value && fits_as_coefficient<T>::value,
                           typename algebraic_traits<T>::field_type>::type
   evaluate(const T& t, const long exp=1) const
   {
      Integer exp_lcm(exp);
      exp_lcm = lcm(denominators(numerator(rf).monomials_as_vector() | denominator(rf).monomials_as_vector()) | exp_lcm);
      const double t_approx = std::pow(convert_to<double>(t),1.0/convert_to<double>(exp_lcm));
      Coefficient base = exp_lcm == exp ? t : t_approx;

      return this->evaluate_exp(base, long(exp_lcm));
   }

   template <typename T>
   typename std::enable_if<std::numeric_limits<Exponent>::is_integer && fits_as_coefficient<T>::value,
                           typename algebraic_traits<T>::field_type>::type
   evaluate(const T& t, const long exp=1) const
   {
      return this->evaluate_exp(t, exp);
   }


   template <typename VectorType, typename T> static
   typename std::enable_if<fits_as_coefficient<T>::value && is_field_of_fractions<Exponent>::value,
                           const LazyVector1<const VectorType&, operations::evaluate<PuiseuxFraction, typename algebraic_traits<T>::field_type> > >::type
   evaluate(const GenericVector<VectorType, PuiseuxFraction>& vec, const T& t, const long exp=1)
   {
      Integer exp_lcm(exp);
      for (auto v = entire(vec.top()); !v.at_end(); ++v)
         exp_lcm = lcm(denominators(numerator(*v).monomials_as_vector() | denominator(*v).monomials_as_vector()) | exp_lcm);

      const double t_approx = std::pow(convert_to<double>(t),1.0/convert_to<double>(exp_lcm));
      const typename algebraic_traits<T>::field_type base = exp_lcm == exp ? t : t_approx;

      return LazyVector1<
               const VectorType&,
               operations::evaluate<PuiseuxFraction, typename algebraic_traits<T>::field_type>
             >(vec.top(), operations::evaluate<PuiseuxFraction, typename algebraic_traits<T>::field_type>(base, long(exp_lcm)));
   }

   template <typename VectorType, typename T> static
   typename std::enable_if<fits_as_coefficient<T>::value && std::numeric_limits<Exponent>::is_integer,
                           const LazyVector1<const VectorType&, operations::evaluate<PuiseuxFraction,T> > >::type
   evaluate(const GenericVector<VectorType,PuiseuxFraction>& vec, const T& t, const long exp=1)
   {
      return LazyVector1<const VectorType&, operations::evaluate<PuiseuxFraction,T> >(vec.top(),operations::evaluate<PuiseuxFraction,T>(t,exp));
   }

   template <typename MatrixType, typename T> static
   typename std::enable_if<fits_as_coefficient<T>::value && is_field_of_fractions<Exponent>::value,
                           const LazyMatrix1<const MatrixType&, operations::evaluate<PuiseuxFraction, typename algebraic_traits<T>::field_type> > >::type
   evaluate(const GenericMatrix<MatrixType, PuiseuxFraction>& m, const T& t, const long exp=1)
   {
      Integer exp_lcm(exp);
      for (auto e = entire(concat_rows(m.top())); !e.at_end(); ++e)
         exp_lcm = lcm(denominators(numerator(*e).monomials_as_vector() | denominator(*e).monomials_as_vector()) | exp_lcm);

      const double t_approx = std::pow(convert_to<double>(t),1.0/convert_to<double>(exp_lcm));
      const typename algebraic_traits<T>::field_type base = exp_lcm == exp ? t : t_approx;

      return LazyMatrix1<
               const MatrixType&,
               operations::evaluate<PuiseuxFraction, typename algebraic_traits<T>::field_type>
             >(m.top(), operations::evaluate<PuiseuxFraction, typename algebraic_traits<T>::field_type>(base, long(exp_lcm)));
   }

   template <typename MatrixType, typename T> static
   typename std::enable_if<fits_as_coefficient<T>::value && std::numeric_limits<Exponent>::is_integer,
                           const LazyMatrix1<const MatrixType&, operations::evaluate<PuiseuxFraction, T> > >::type
   evaluate(const GenericMatrix<MatrixType, PuiseuxFraction>& m, const T& t, const long exp=1)
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


   template <typename Scalar, 
             typename=typename std::enable_if<can_initialize<Coefficient, Scalar>::value>::type>
   explicit operator Scalar () const
   {
      if (denominator(*this).is_one() &&
          numerator(*this).deg() == 0 && numerator(*this).lower_deg() == 0)
         return static_cast<Scalar>(numerator(*this).lc());

      throw std::runtime_error("Conversion to scalar not possible.");
   }

   explicit operator TropicalNumber<MinMax, Exponent> () const
   {
      return val();
   }

   size_t get_hash() const noexcept { return rf.get_hash(); }

#if POLYMAKE_DEBUG
   void dump() const __attribute__((used)) { rf.dump(); }
#endif
};

template <typename MinMax, typename Coefficient, typename Exponent>
inline
int sign(const PuiseuxFraction<MinMax, Coefficient, Exponent>& x)
{
   return x.compare(zero_value<Coefficient>());
}

namespace operations {

template <typename OpRef, typename T>
struct evaluate {
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
struct evaluate<OpRef, double> {
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

}

template <typename MinMax, typename Coefficient, typename Exponent>
struct algebraic_traits< PuiseuxFraction<MinMax, Coefficient, Exponent> > {
   typedef PuiseuxFraction<MinMax,typename algebraic_traits<Coefficient>::field_type,Exponent> field_type;
};


template <typename MinMax, typename Coefficient, typename Exponent>
struct spec_object_traits< Serialized< PuiseuxFraction<MinMax, Coefficient, Exponent> > >
   : spec_object_traits<is_composite> {

   typedef PuiseuxFraction<MinMax, Coefficient, Exponent> masquerade_for;

   typedef RationalFunction<Coefficient, Exponent> elements;

   template <typename Me, typename Visitor>
   static void visit_elements(Me& me, Visitor& v)
   {
      v << me.rf;
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
      return pm::is_zero(p.rf);
   }

   static
   bool is_one(const persistent_type& p)
   {
      return pm::is_one(p.rf);
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

#endif // POLYMAKE_VALUATED_RATIONAL_FUNCTION_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
