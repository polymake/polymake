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
   $Project: polymake $$Id$
*/

/** @file TropicalNumber.h
    @brief Implementation of pm::TropicalNumber class
*/


#ifndef POLYMAKE_TROPICALNUMBER_H
#define POLYMAKE_TROPICALNUMBER_H

#include "polymake/internal/converters.h"
#include "polymake/Polynomial.h"
#include "polymake/Integer.h"

namespace pm {

/**
 * Defines Min and Max such that they can be used as Addition by TropicalNumber
 */  
  
struct Max;

struct Min {
   template <typename T>
   static const T& apply(const T& x, const T& y) { return std::min(x,y); } 

   static const int orientation() { return 1; }

   typedef Max dual;
};

struct Max {
   template <typename T>
   static const T& apply(const T& x, const T& y) { return std::max(x,y); }
   
   static const int orientation() { return -1; }

   typedef Min dual;
};

/**
 * TropicalNumber models a modification of a chosen scalar type. Its multiplication is the 
 * inherent addition of the scalar, the addition is passed as a template parameter (Usually it is
 * Max or Min). The Addition must have methods apply(x,y) and orientation(). The latter determines
 * the tropical zero of the TropicalNumber type: It is Addition::orientation() * infinity.
 * 
 * The Scalar has to know its zero and its own infinity value via std::numeric_limits.
 */
template <typename Addition, typename Scalar = Rational>
class TropicalNumber : public Scalar {
  
  friend struct spec_object_traits<TropicalNumber<Addition,Scalar> >;
  
  friend struct std::numeric_limits<TropicalNumber<Addition,Scalar> >;

  
public:
    
   // *** CONSTRUCTORS ***
  
   TropicalNumber() : Scalar(convert_to<Scalar>(zero())) { }
   explicit TropicalNumber(Scalar x) : Scalar(x) {}

   // *** ZERO ***
   
   static const TropicalNumber<Addition, Scalar> zero() {
      return spec_object_traits<TropicalNumber<Addition,Scalar> >::zero();
   }
   
   // *** ONE ***
   
   static const TropicalNumber<Addition, Scalar> one() {
      return spec_object_traits<TropicalNumber<Addition,Scalar> >::one();
   }
   
   // *** METHODS ***
   
   
   const int orientation() {
      return Addition::orientation();
   }
   
   // *** OPERATOR OVERLOADING ***
   
   // -- assignment --
   
   TropicalNumber<Addition,Scalar>& operator= (const int &a) {
     Scalar::operator=(a);
     return *this;
   }
   
   TropicalNumber<Addition,Scalar>& operator= (const double &a) {
     Scalar::operator=(a);
     return *this;
   }
   
   TropicalNumber<Addition,Scalar>& operator= (const long &a) {
     Scalar::operator=(a);
     return *this;
   }
   
   // -- binary --
   
   friend TropicalNumber<Addition,Scalar> operator+ (const TropicalNumber<Addition,Scalar>& a, const TropicalNumber<Addition,Scalar>& b)
   {
      return Addition::apply(a,b);
   }
   
   friend TropicalNumber<Addition,Scalar> operator* (const TropicalNumber<Addition,Scalar>& a, const TropicalNumber<Addition,Scalar>& b)
   {
      return TropicalNumber<Addition,Scalar>( convert_to<Scalar>(a) + convert_to<Scalar>(b));
   }
   
   friend TropicalNumber<Addition,Scalar> operator/ (const TropicalNumber<Addition,Scalar>& a, const TropicalNumber<Addition,Scalar>& b)
   {
      return TropicalNumber<Addition,Scalar>( convert_to<Scalar>(a) - convert_to<Scalar>(b));
   }
   
   
   // The following are disallowed. We overload them to prevent calling any version
   // Scalar may have overloaded. The complicated templating takes care that only 
   // operations with scalar types are blocked (i.e. polynomials and vectors, matrices are
   // allowed)
   
   template <typename T>
   friend typename 
   enable_if<TropicalNumber<Addition,Scalar>,identical<typename object_traits<T>::model, is_scalar>::value>::type operator+(const TropicalNumber<Addition,Scalar> &a,
						    const T& b){
      throw std::runtime_error("Tropical addition is only applicable to explicitly declared TropicalNumbers");
   }
   
   template <typename T>
   friend typename 
   enable_if<TropicalNumber<Addition,Scalar>,identical<typename object_traits<T>::model, is_scalar>::value>::type operator+(const T& b, 
						    const TropicalNumber<Addition,Scalar> &a){
      throw std::runtime_error("Tropical addition is only applicable to explicitly declared TropicalNumbers");
   }
   
   friend TropicalNumber<Addition,Scalar> operator-(const TropicalNumber<Addition,Scalar> &a,
                                                   const TropicalNumber<Addition,Scalar> &b) {
      throw std::runtime_error("There is no tropical subtraction.");
   }
   
   template <typename T>
   friend TropicalNumber<Addition,Scalar> operator-(const TropicalNumber<Addition,Scalar> &a,
						    const T& b){
      throw std::runtime_error("There is no tropical subtraction.");
   }
 
   template <typename T>
   friend TropicalNumber<Addition,Scalar> operator-(const T& b, 
						    const TropicalNumber<Addition,Scalar> &a){
      throw std::runtime_error("There is no tropical subtraction.");
   }
 
   template <typename T>
   friend typename 
   enable_if<TropicalNumber<Addition,Scalar>,identical<typename object_traits<T>::model, is_scalar>::value>::type operator*(const TropicalNumber<Addition,Scalar> &a,
						    const T& b){
      throw std::runtime_error("Tropical multiplication  only applicable to explicitly declared TropicalNumbers");
   } 
   
   template <typename T>
   friend typename 
   enable_if<TropicalNumber<Addition,Scalar>,identical<typename object_traits<T>::model, is_scalar>::value>::type operator*(const T& b, 
						    const TropicalNumber<Addition,Scalar> &a){
      throw std::runtime_error("Tropical multiplication  only applicable to explicitly declared TropicalNumbers");
   }
   
   template <typename T>
   friend typename 
   enable_if<TropicalNumber<Addition,Scalar>,identical<typename object_traits<T>::model, is_scalar>::value>::type operator/(const TropicalNumber<Addition,Scalar> &a,
						    const T& b){
      throw std::runtime_error("Tropical division is only applicable to explicitly declared TropicalNumbers");
   }
   
   template <typename T>
   friend typename 
   enable_if<TropicalNumber<Addition,Scalar>,identical<typename object_traits<T>::model, is_scalar>::value>::type operator/(const T&b, const TropicalNumber<Addition,Scalar> &a){
      throw std::runtime_error("Tropical division is only applicable to explicitly declared TropicalNumbers");
   }
 
   
   // -- unary --
   
   TropicalNumber<Addition,Scalar>& operator++ (int)
   {
      *this = *this + TropicalNumber<Addition,Scalar>(spec_object_traits<Scalar>::zero());
      return *this;
   }

   TropicalNumber<Addition,Scalar>& operator-- (int)
   {
      throw std::runtime_error("There is no tropical subtraction");
   }

   /// In-place negation.
   TropicalNumber<Addition,Scalar>& operator-() const
   {
      throw std::runtime_error("There is no tropical subtraction");
   }

   /// Addition %operator.
   TropicalNumber<Addition,Scalar>& operator+= (const TropicalNumber<Addition,Scalar>& b)
   {
      *this = *this + b;
      return *this;
   }

   /// Subtraction %operator.
   TropicalNumber<Addition,Scalar>& operator-= (const TropicalNumber<Addition,Scalar>& b)
   {
      throw std::runtime_error("There is no tropical subtraction.");
   }


   /// Multiplication %operator.
   TropicalNumber<Addition,Scalar>& operator*= (const TropicalNumber<Addition,Scalar>& b)
   {
      *this = *this * b;
      return *this;
   }

   /// Division %operator.
   TropicalNumber<Addition,Scalar>& operator/= (const TropicalNumber<Addition,Scalar>& b)
   {
      *this = *this / b;
      return *this;
   }

   
};

/*
 * Class converters for TropicalNumber (which all just use Scalar's 
 * conversion)
 */

//template <typename Addition, typename Scalar >
template <typename Addition, typename Scalar>
class conv<TropicalNumber<Addition,Scalar>, int> {
public:
   typedef TropicalNumber<Addition,Scalar> argument_type;
   typedef int result_type;
   result_type operator() (const TropicalNumber<Addition,Scalar>& a) const { return a.to_int(); }
};

template <typename Addition, typename Scalar>
class conv<TropicalNumber<Addition,Scalar>, long> {
public:
   typedef TropicalNumber<Addition,Scalar> argument_type;
   typedef long result_type;
   result_type operator() (const TropicalNumber<Addition,Scalar>& a) const { return a.to_long(); }
};

template <typename Addition, typename Scalar>
class conv<TropicalNumber<Addition,Scalar>, double> {
public:
   typedef TropicalNumber<Addition,Scalar> argument_type;
   typedef double result_type;
   result_type operator() (const TropicalNumber<Addition,Scalar>& a) const { return a.to_double(); }
};

template <typename Addition, typename Scalar>
class conv<TropicalNumber<Addition,Scalar>, std::string> {
public:
   typedef TropicalNumber<Addition,Scalar> argument_type;
   typedef std::string result_type;
   result_type operator() (const TropicalNumber<Addition,Scalar>& a) const { return a.to_string(); }
};

template <typename Addition, typename Scalar>
class conv<TropicalNumber<Addition,Scalar>, Integer> : public conv_by_cast<TropicalNumber<Addition,Scalar>, Integer> {};
   

/**
 * Object traits for TropicalNumber: Defines when a TropicalNumber is tropically zero/one and
 * what tropical zero and one are.
 */
template <typename Addition, typename Scalar>
struct spec_object_traits<TropicalNumber<Addition, Scalar> > : spec_object_traits<is_scalar> {
  
   
   static
   bool is_zero(const TropicalNumber<Addition,Scalar>& a)
   {
      return (Addition::orientation() * isinf(a)) == 1;
   }

   static
   bool is_one(const TropicalNumber<Addition,Scalar>& a)
   {
      return spec_object_traits<Scalar>().is_zero( a );
   }

   static const TropicalNumber<Addition,Scalar>& zero()
   {
      static const TropicalNumber<Addition,Scalar> t_zero(Addition::orientation() * std::numeric_limits<Scalar>().infinity());
      return t_zero;
   }

   static const TropicalNumber<Addition,Scalar>& one()
   {
     static const TropicalNumber<Addition,Scalar> t_one(spec_object_traits<Scalar>::zero());
      return t_one;
   }

};


}

namespace std {

template <typename Addition, typename Scalar>
struct numeric_limits<pm::TropicalNumber<Addition, Scalar> > : numeric_limits<Scalar> {
   static pm::TropicalNumber<Addition,Scalar> min() throw() { return pm::TropicalNumber<Addition,Scalar>(pm::maximal<Scalar>(),-1); }
   static pm::TropicalNumber<Addition,Scalar> infinity() throw() { return pm::TropicalNumber<Addition,Scalar>(pm::maximal<Scalar>()); }
   static pm::TropicalNumber<Addition,Scalar> max() throw() { return pm::TropicalNumber<Addition,Scalar>(pm::maximal<Scalar>()); }
};

}



namespace polymake {
   using pm::Min;
   using pm::Max;
   using pm::TropicalNumber;
}

#endif // POLYMAKE_TROPICALNUMBER_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
