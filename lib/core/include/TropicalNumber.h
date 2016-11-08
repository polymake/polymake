/* Copyright (c) 1997-2016
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

/** @file TropicalNumber.h
    @brief Implementation of pm::TropicalNumber class
*/


#ifndef POLYMAKE_TROPICALNUMBER_H
#define POLYMAKE_TROPICALNUMBER_H

#include "polymake/internal/converters.h"
#include "polymake/internal/comparators.h"
#include "polymake/Rational.h"
#include "polymake/GenericIO.h"

namespace pm {

/**
 * Defines Min and Max such that they can be used as Addition by TropicalNumber
 */

struct Max;

struct Min {
   template <typename T>
   static
   const T& apply(const T& x, const T& y)
   {
      return std::min(x,y);
   }

   template <typename T, typename T2>
   static
   T& assign(T& x, T2&& y)
   {
      if (x > y) x=std::forward<T2>(y);
      return x;
   }

   static
   constexpr int orientation() { return 1; }

   typedef Max dual;
};

struct Max {
   template <typename T>
   static
   const T& apply(const T& x, const T& y)
   {
      return std::max(x,y);
   }

   template <typename T, typename T2>
   static
   T& assign(T& x, T2&& y)
   {
      if (x < y) x=std::forward<T2>(y);
      return x;
   }

   static
   constexpr int orientation() { return -1; }

   typedef Min dual;
};

template <typename Addition, typename Scalar>
class TropicalNumber;

template <typename Addition, typename Scalar>
int isinf(const TropicalNumber<Addition, Scalar>& x) noexcept;

/**
 * TropicalNumber models a modification of a chosen scalar type. Its multiplication is the
 * inherent addition of the scalar, the addition is passed as a template parameter (Usually it is
 * Max or Min). The Addition must have methods apply(x,y), assign(x,y), and orientation(). The latter determines
 * the tropical zero of the TropicalNumber type: It is Addition::orientation() * infinity.
 *
 * The Scalar has to know its zero and its own infinity value via std::numeric_limits.
 */
template <typename Addition, typename Scalar = Rational>
class TropicalNumber {
public:
   template <typename T>
   struct can_initialize_scalar
      : bool_constant<can_initialize<pure_type_t<T>, Scalar>::value && !std::is_same<pure_type_t<T>, TropicalNumber>::value> {};

   template <typename T>
   struct can_assign_scalar
      : bool_constant<isomorphic_types<pure_type_t<T>, Scalar>::value && can_assign_to<pure_type_t<T>, Scalar>::value && !std::is_same<pure_type_t<T>, TropicalNumber>::value> {};

   template <typename T>
   struct fits_as_scalar
      : bool_constant<can_upgrade<pure_type_t<T>, Scalar>::value && !std::is_same<pure_type_t<T>, TropicalNumber>::value> {};

   template <typename T>
   struct fits_as_operand
      : bool_constant<can_upgrade<pure_type_t<T>, Scalar>::value || std::is_same<pure_type_t<T>, TropicalNumber>::value> {};

   // *** CONSTRUCTORS ***

   // initialize with tropical zero
   TropicalNumber()
      : TropicalNumber(zero()) {}

   TropicalNumber(const TropicalNumber&) = default;
   TropicalNumber(TropicalNumber&&) = default;

   template <typename T, typename=typename std::enable_if<can_initialize_scalar<T>::value>::type>
   explicit TropicalNumber(T&& x)
      : scalar(std::forward<T>(x)) {}

   // *** ZERO ***

   static
   const TropicalNumber& zero()
   {
      return spec_object_traits<TropicalNumber>::zero();
   }

   // *** DUAL_ZERO ***

   static
   const TropicalNumber& dual_zero()
   {
      return spec_object_traits<TropicalNumber>::dual_zero();
   }

   // *** ONE ***

   static
   const TropicalNumber& one()
   {
      return spec_object_traits<TropicalNumber>::one();
   }

   // *** METHODS ***

   static
   int orientation() noexcept
   {
      return Addition::orientation();
   }

   explicit operator const Scalar& () const noexcept { return scalar; }

   template <typename T,
             typename=typename std::enable_if<!std::is_same<T, Scalar>::value && can_initialize<Scalar, T>::value>::type>
   explicit operator T () const
   {
      return static_cast<T>(scalar);
   }

   // *** assignment ***

   TropicalNumber& operator= (const TropicalNumber&) = default;
   TropicalNumber& operator= (TropicalNumber&&) = default;

   template <typename T, typename=typename std::enable_if<can_assign_scalar<T>::value>::type>
   TropicalNumber& operator= (T&& a)
   {
      scalar=std::forward<T>(a);
      return *this;
   }

   void swap(TropicalNumber& a)
   {
      std::swap(scalar, a.scalar);
   }

   // *** addition ***

   // looks suspicious... shouldn't it call one() instead?
   TropicalNumber& operator++ (int)
   {
      Addition::assign(scalar, spec_object_traits<Scalar>::zero());
      return *this;
   }

   TropicalNumber& operator+= (const TropicalNumber& b)
   {
      Addition::assign(scalar, b.scalar);
      return *this;
   }

   TropicalNumber& operator+= (TropicalNumber&& b)
   {
      Addition::assign(scalar, std::move(b.scalar));
      return *this;
   }

   friend
   TropicalNumber operator+ (const TropicalNumber& a, const TropicalNumber& b)
   {
      return TropicalNumber(Addition::apply(a.scalar, b.scalar));
   }

   friend
   TropicalNumber&& operator+ (TropicalNumber&& a, const TropicalNumber& b)
   {
      return std::move(a+=b);
   }

   friend
   TropicalNumber&& operator+ (const TropicalNumber& a, TropicalNumber&& b)
   {
      return std::move(b+=a);
   }

   friend
   TropicalNumber&& operator+ (TropicalNumber&& a, TropicalNumber&& b)
   {
      return std::move(a+=std::move(b));
   }

   // *** multiplication ***

   TropicalNumber& operator*= (const TropicalNumber& b)
   {
      if (!std::numeric_limits<Scalar>::has_infinity) {
         const int i1=isinf(*this), i2=isinf(b);
         if (i1 || i2) {
            if (i1+i2==0) throw GMP::NaN();
            if (!i1) *this=b;
            return *this;
         }
      }
      scalar += b.scalar;
      return *this;
   }

   friend
   TropicalNumber operator* (const TropicalNumber& a, const TropicalNumber& b)
   {
      if (!std::numeric_limits<Scalar>::has_infinity) {
         const int i1=isinf(a), i2=isinf(b);
         if (i1 || i2) {
            if (i1+i2==0) throw GMP::NaN();
            return i1 ? a : b;
         }
      }
      return TropicalNumber(a.scalar + b.scalar);
   }

   friend
   TropicalNumber&& operator* (TropicalNumber&& a, const TropicalNumber& b)
   {
      return std::move(a*=b);
   }

   friend
   TropicalNumber&& operator* (const TropicalNumber& a, TropicalNumber&& b)
   {
      return std::move(b*=a);
   }

   friend
   TropicalNumber&& operator* (TropicalNumber&& a, TropicalNumber&& b)
   {
      return std::move(a*=b);
   }

   // *** division ***

   TropicalNumber& operator/= (const TropicalNumber& b)
   {
      if (!std::numeric_limits<Scalar>::has_infinity) {
         const int i1=isinf(*this), i2=isinf(b);
         if (i1 || i2) {
            if (i1==i2) throw GMP::NaN();
            if (!i1) scalar=-b.scalar;
         }
      }
      scalar -= b.scalar;
      return *this;
   }

   friend
   TropicalNumber operator/ (const TropicalNumber& a, const TropicalNumber& b)
   {
      if (!std::numeric_limits<Scalar>::has_infinity) {
         const int i1=isinf(a), i2=isinf(b);
         if (i1 || i2) {
            if (i1==i2) throw GMP::NaN();
            return TropicalNumber(i1 ? a.scalar : -b.scalar);
         }
      }
      return TropicalNumber(a.scalar - b.scalar);
   }

   friend
   TropicalNumber&& operator/ (TropicalNumber&& a, const TropicalNumber& b)
   {
      return std::move(a/=b);
   }

   friend
   TropicalNumber&& operator/ (const TropicalNumber& a, TropicalNumber&& b)
   {
      b /= a;
      negate(b.scalar);
      return std::move(b);
   }

   friend
   TropicalNumber&& operator/ (TropicalNumber&& a, TropicalNumber&& b)
   {
      return std::move(a/=b);
   }

   // *** comparisons ***

   int compare(const TropicalNumber& a) const
   {
      return operations::cmp()(scalar, a.scalar);
   }

   friend
   bool operator== (const TropicalNumber& a, const TropicalNumber& b)
   {
      return a.scalar == b.scalar;
   }

   template <typename T, typename=typename std::enable_if<fits_as_scalar<T>::value>::type>
   friend
   bool operator== (const TropicalNumber& a, const T& b)
   {
      return a.scalar==b;
   }

   template <typename T, typename=typename std::enable_if<fits_as_scalar<T>::value>::type>
   friend
   bool operator== (const T& a, const TropicalNumber& b)
   {
      return b==a;
   }

   template <typename T, typename=typename std::enable_if<fits_as_operand<T>::value>::type>
   friend
   bool operator!= (const TropicalNumber& a, const T& b)
   {
      return !(a==b);
   }

   template <typename T, typename=typename std::enable_if<fits_as_scalar<T>::value>::type>
   friend
   bool operator!= (const T& a, const TropicalNumber& b)
   {
      return !(b==a);
   }

   friend
   bool operator< (const TropicalNumber& a, const TropicalNumber& b)
   {
      return a.scalar < b.scalar;
   }

   friend
   bool operator> (const TropicalNumber& a, const TropicalNumber& b)
   {
      return b<a;
   }

   friend
   bool operator<= (const TropicalNumber& a, const TropicalNumber& b)
   {
      return !(b<a);
   }

   friend
   bool operator>= (const TropicalNumber& a, const TropicalNumber& b)
   {
      return !(a<b);
   }

   // *** input/output ***

   template <typename Input>
   friend
   Input& operator>> (GenericInput<Input>& in, TropicalNumber& me)
   {
      return me.read(in.top(), bool_constant<is_derived_from_instance_of<Input, PlainParser>::value && !std::numeric_limits<Scalar>::has_infinity>());
   }

   template <typename Output>
   friend
   Output& operator<< (GenericOutput<Output>& out, const TropicalNumber& me)
   {
      return me.write(out.top(), bool_constant<is_derived_from_instance_of<Output, PlainPrinter>::value && !std::numeric_limits<Scalar>::has_infinity>());
   }

protected:
   template <typename Input>
   Input& read(Input& in, std::false_type)
   {
      return in >> scalar;
   }

   template <typename Input>
   Input& read(Input& in, std::true_type)
   {
      if (const int inf=in.probe_inf())
         scalar=inf*std::numeric_limits<Scalar>::max();
      else
         in >> scalar;
      return in;
   }

   template <typename Output>
   Output& write(Output& out, std::false_type) const
   {
      return out << scalar;
   }

   template <typename Output>
   Output& write(Output& out, std::true_type) const
   {
      if (const int inf=isinf(*this))
         out << (inf>0 ? "inf" : "-inf");
      else
         out << scalar;
      return out;
   }

   /// numerical value
   Scalar scalar;
};

/**
 * Object traits for TropicalNumber: Defines when a TropicalNumber is tropically zero/one and
 * what tropical zero and one are.
 */
template <typename Addition, typename Scalar>
struct spec_object_traits<TropicalNumber<Addition, Scalar> >
   : spec_object_traits<is_scalar> {

   static
   bool is_zero(const TropicalNumber<Addition, Scalar>& a) noexcept(noexcept(isinf(std::declval<Scalar>())))
   {
      return std::numeric_limits<Scalar>::has_infinity
             ? Addition::orientation() * isinf(static_cast<const Scalar&>(a)) == 1
             : static_cast<const Scalar&>(a) == Addition::orientation() * std::numeric_limits<Scalar>::max();
   }

   static
   bool is_one(const TropicalNumber<Addition, Scalar>& a) noexcept(noexcept(pm::is_zero(std::declval<Scalar>())))
   {
      return pm::is_zero(static_cast<const Scalar&>(a));
   }

   static
   const TropicalNumber<Addition, Scalar>& zero()
   {
      static const TropicalNumber<Addition, Scalar> t_zero(Addition::orientation() *
                                                           (std::numeric_limits<Scalar>::has_infinity
                                                            ? std::numeric_limits<Scalar>::infinity()
                                                            : std::numeric_limits<Scalar>::max()));
      return t_zero;
   }

   static const TropicalNumber<Addition, Scalar>& dual_zero()
   {
      static const TropicalNumber<Addition, Scalar> t_d_zero(-Addition::orientation() *
                                                             (std::numeric_limits<Scalar>::has_infinity
                                                              ? std::numeric_limits<Scalar>::infinity()
                                                              : std::numeric_limits<Scalar>::max()));
      return t_d_zero;
   }

   static const TropicalNumber<Addition, Scalar>& one()
   {
      static const TropicalNumber<Addition, Scalar> t_one(spec_object_traits<Scalar>::zero());
      return t_one;
   }
};

template <typename Addition, typename Scalar>
inline
bool isfinite(const TropicalNumber<Addition, Scalar>& x) noexcept
{
   return std::numeric_limits<Scalar>::has_infinity
          ? isfinite(static_cast<const Scalar&>(x))
          : static_cast<const Scalar&>(x) != std::numeric_limits<Scalar>::max() &&
            static_cast<const Scalar&>(x) != std::numeric_limits<Scalar>::min();
}

template <typename Addition, typename Scalar>
inline
int isinf(const TropicalNumber<Addition, Scalar>& x) noexcept
{
   return std::numeric_limits<Scalar>::has_infinity
          ? isinf(static_cast<const Scalar&>(x)) :
          static_cast<const Scalar&>(x) == std::numeric_limits<Scalar>::min()
          ? -1 : static_cast<const Scalar&>(x) == std::numeric_limits<Scalar>::max();
}

// for comparisons with implicit zero in sparse containers
template <typename Addition, typename Scalar>
inline
int sign(const TropicalNumber<Addition, Scalar>& x) noexcept
{
   return is_zero(x) ? 0 : -Addition::orientation();
}

template <typename TPrimitive, typename Addition, typename Scalar>
constexpr
typename std::enable_if<std::is_arithmetic<Scalar>::value, TPrimitive>::type
max_value_as(mlist<TropicalNumber<Addition, Scalar>>)
{
   return max_value_as<TPrimitive>(mlist<Scalar>());
}

template <typename TPrimitive, typename Addition, typename Scalar>
constexpr
typename std::enable_if<std::is_arithmetic<Scalar>::value, TPrimitive>::type
min_value_as(mlist<TropicalNumber<Addition, Scalar>>)
{
   return min_value_as<TPrimitive>(mlist<Scalar>());
}

}

namespace std {

template <typename Addition, typename Scalar>
inline
void swap(pm::TropicalNumber<Addition, Scalar>& a, pm::TropicalNumber<Addition, Scalar>& b)
{
   a.swap(b);
}

template <typename Addition, typename Scalar>
class numeric_limits<pm::TropicalNumber<Addition, Scalar>>
   : public numeric_limits<Scalar> {
   typedef numeric_limits<Scalar> base_t;
public:
   typedef pm::TropicalNumber<Addition, Scalar> value_type;

   static const bool has_infinity=true;
   static value_type min() noexcept(base_t::min()) { return value_type(base_t::min()); }
   static value_type infinity() noexcept(base_t::infinity()) { return value_type(-Addition::orientation() * base_t::infinity()); }
   static value_type max() noexcept(base_t::max()) { return value_type(base_t::max()); }
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
