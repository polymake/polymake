/* Copyright (c) 1997-2022
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

#include "polymake/internal/type_manip.h"
#include <cmath>
#include <string>
#include <functional>

namespace pm {

template <typename T>
std::enable_if_t<std::is_arithmetic<T>::value, T&>
negate(T& x)
{
   x=-x;
   return x;
}

template <typename T>
std::enable_if_t<is_class_or_union<pure_type_t<T>>::value &&
                 !std::is_const<std::remove_reference_t<T>>::value, T&&>
negate(T&& x)
{
   return std::forward<T>(x.negate());
}

template <typename T>
std::enable_if_t<std::is_integral<T>::value, T&>
complement(T& x)
{
   x=~x;
   return x;
}

template <typename T, typename=std::enable_if_t<std::is_same<typename object_traits<T>::generic_tag, is_scalar>::value &&
                                                std::is_same<decltype(std::declval<const T&>() * std::declval<const T&>()), T>::value>>
T sqr(const T& x)
{
   return x*x;
}

namespace operations {

struct partial {};
struct partial_left : partial {};
struct partial_right : partial {};

#define GuessResultType(name, sign) \
template <typename T1, typename T2> \
struct name##_result { \
   using type = decltype(std::declval<const T1&>() sign std::declval<const T2&>()); \
}

GuessResultType(add,+);
GuessResultType(sub,-);
GuessResultType(mul,*);
GuessResultType(div,/);

template <typename Op, typename Result>
struct neg_scalar {
   typedef Op argument_type;
   typedef Result result_type;
   result_type operator() (typename function_argument<Op>::type a) const { return -a; }
   void assign(Op& a) const { negate(a); }
};

template <typename Op>
struct neg_scalar<Op,void> {
   typedef void result_type;
};

template <typename Left, typename Right, typename Result>
struct add_scalar {
   typedef Left first_argument_type;
   typedef Right second_argument_type;
   typedef Result result_type;

   result_type operator() (typename function_argument<Left>::type a, typename function_argument<Right>::type b) const { return a+b; }
   template <typename Iterator2>
   const Left& operator() (partial_left, const Left& a, const Iterator2&) const { return a; }
   template <typename Iterator1>
   const Right& operator() (partial_right, const Iterator1&, const Right& b) const { return b; }
   void assign(Left& a, typename function_argument<Right>::type b) const { a+=b; }
};

template <typename Left, typename Right, typename Result>
struct sub_scalar {
   typedef Left first_argument_type;
   typedef Right second_argument_type;
   typedef Result result_type;

   result_type operator() (typename function_argument<Left>::type a, typename function_argument<Right>::type b) const { return a-b; }
   template <typename Iterator2>
   const Left& operator() (partial_left, const Left& a, const Iterator2&) const { return a; }
   template <typename Iterator1>
   result_type operator() (partial_right, const Iterator1&, typename function_argument<Right>::type b) const { return -b; }
   void assign(Left& a, typename function_argument<Right>::type b) const { a-=b; }
};

template <typename Left, typename Right, typename Result>
struct mul_scalar {
   typedef Left first_argument_type;
   typedef Right second_argument_type;
   typedef Result result_type;

   result_type operator() (typename function_argument<Left>::type a, typename function_argument<Right>::type b) const { return a*b; }
   void assign(Left& a, typename function_argument<Right>::type b) const { a*=b; }
};

template <typename Left, typename Right>
struct mul_scalar<Left,Right,void> {
   typedef void result_type;
};

template <typename Left, typename Right, typename Result>
struct div_scalar {
   typedef Left first_argument_type;
   typedef Right second_argument_type;
   typedef Result result_type;

   result_type operator() (typename function_argument<Left>::type a, typename function_argument<Right>::type b) const { return a/b; }
   void assign(Left& a, typename function_argument<Right>::type b) const { a/=b; }
};

template <typename Left, typename Right>
struct div_scalar<Left,Right,void> {
   typedef void result_type;
};

template <typename Left, typename Right, typename Result>
struct divexact_scalar : div_scalar <Left, Right, Result> {};

} // end namespace operations

template <typename Char, typename Traits, typename Alloc>
struct spec_object_traits< std::basic_string<Char, Traits, Alloc> >
   : spec_object_traits<is_opaque> {};

using std::sqrt;

}
namespace polymake {

using pm::negate;
using pm::complement;
using pm::sqr;
using pm::sqrt;

// TODO: rename back to operations when the old stuff has gone
namespace cleanOperations {

using neg = std::negate<>;
using add = std::plus<>;
using sub = std::minus<>;
using mul = std::multiplies<>;
using div = std::divides<>;
using mod = std::modulus<>;
using bit_not = std::bit_not<>;
using bit_and = std::bit_and<>;
using bit_or = std::bit_or<>;
using bit_xor = std::bit_xor<>;

struct lshift {
   template <typename Left, typename Right>
   decltype(auto) operator() (Left&& l, Right&& r) const
   {
      return std::forward<Left>(l) << std::forward<Right>(r);
   }
};

struct rshift {
   template <typename Left, typename Right>
   decltype(auto) operator() (Left&& l, Right&& r) const
   {
      return std::forward<Left>(l) >> std::forward<Right>(r);
   }
};

/// check whether the given elementary operation is defined for given argument types
template <typename Operation, typename T1, typename T2=void, typename is_defined=void>
struct can : std::false_type {};

template <typename T>
struct can<neg, T, void, accept_valid_type<decltype(-std::declval<const T&>())>> : std::true_type {};

template <typename T1, typename T2>
struct can<add, T1, T2, accept_valid_type<decltype(std::declval<const T1&>() + std::declval<const T2&>())>> : std::true_type {
   using type = decltype(std::declval<const T1&>() + std::declval<const T2&>());
};

template <typename T1, typename T2>
struct can<sub, T1, T2, accept_valid_type<decltype(std::declval<const T1&>() - std::declval<const T2&>())>> : std::true_type {
   using type = decltype(std::declval<const T1&>() - std::declval<const T2&>());
};

template <typename T1, typename T2>
struct can<mul, T1, T2, accept_valid_type<decltype(std::declval<const T1&>() * std::declval<const T2&>())>> : std::true_type {
   using type = decltype(std::declval<const T1&>() * std::declval<const T2&>());
};

template <typename T1, typename T2>
struct can<div, T1, T2, accept_valid_type<decltype(std::declval<const T1&>() / std::declval<const T2&>())>> : std::true_type {
   using type = decltype(std::declval<const T1&>() / std::declval<const T2&>());
};

template <typename T1, typename T2>
struct can<mod, T1, T2, accept_valid_type<decltype(std::declval<const T1&>() % std::declval<const T2&>())>> : std::true_type {
   using type = decltype(std::declval<const T1&>() % std::declval<const T2&>());
};

template <typename T>
struct can<bit_not, T, void, accept_valid_type<decltype(~std::declval<const T&>())>> : std::true_type {};

template <typename T1, typename T2>
struct can<bit_and, T1, T2, accept_valid_type<decltype(std::declval<const T1&>() & std::declval<const T2&>())>> : std::true_type {
   using type = decltype(std::declval<const T1&>() & std::declval<const T2&>());
};

template <typename T1, typename T2>
struct can<bit_or, T1, T2, accept_valid_type<decltype(std::declval<const T1&>() | std::declval<const T2&>())>> : std::true_type {
   using type = decltype(std::declval<const T1&>() | std::declval<const T2&>());
};

template <typename T1, typename T2>
struct can<bit_xor, T1, T2, accept_valid_type<decltype(std::declval<const T1&>() ^ std::declval<const T2&>())>> : std::true_type {
   using type = decltype(std::declval<const T1&>() ^ std::declval<const T2&>());
};

template <typename T1, typename T2>
struct can<lshift, T1, T2, accept_valid_type<decltype(std::declval<const T1&>() << std::declval<const T2&>())>> : std::true_type {
   using type = decltype(std::declval<const T1&>() << std::declval<const T2&>());
};

template <typename T1, typename T2>
struct can<rshift, T1, T2, accept_valid_type<decltype(std::declval<const T1&>() >> std::declval<const T2&>())>> : std::true_type {
   using type = decltype(std::declval<const T1&>() >> std::declval<const T2&>());
};


/// execute the assignment flavor of the given operation
/// that is, the result is to be assigned to the left operand
/// by default a dedicated method assign() is assumed to the operation class
template <typename Operation>
struct assign : public Operation {
   template <typename Left, typename Right>
   decltype(auto) operator() (Left&& l, Right&& r) const
   {
      return Operation::assign(std::forward<Left>(l), std::forward<Right>(r));
   }
};

/// temporarily cast the given opration to its assignment flavor
template <typename Operation>
const assign<Operation>& assignment_flavor_of(const Operation& op)
{
   return static_cast<const assign<Operation>&>(op);
}

template <>
struct assign<neg> : neg {
   template <typename T>
   decltype(auto) operator() (T&& x) const
   {
      return pm::negate(std::forward<T>(x));
   }
};

template <>
struct assign<add> : add {
   template <typename Left, typename Right>
   decltype(auto) operator() (Left&& l, Right&& r) const
   {
      return std::forward<Left>(l += std::forward<Right>(r));
   }
};

template <>
struct assign<sub> : sub {
   template <typename Left, typename Right>
   decltype(auto) operator() (Left&& l, Right&& r) const
   {
      return std::forward<Left>(l -= std::forward<Right>(r));
   }
};

template <>
struct assign<mul> : mul {
   template <typename Left, typename Right>
   decltype(auto) operator() (Left&& l, Right&& r) const
   {
      return std::forward<Left>(l *= std::forward<Right>(r));
   }
};

template <>
struct assign<div> : div {
   template <typename Left, typename Right>
   decltype(auto) operator() (Left&& l, Right&& r) const
   {
      return std::forward<Left>(l /= std::forward<Right>(r));
   }
};

template <>
struct assign<mod> : mod {
   template <typename Left, typename Right>
   decltype(auto) operator() (Left&& l, Right&& r) const
   {
      return std::forward<Left>(l %= std::forward<Right>(r));
   }
};

template <>
struct assign<bit_not> : bit_not {
   template <typename T>
   decltype(auto) operator() (T&& x) const
   {
      return complement(std::forward<T>(x));
   }
};

template <>
struct assign<bit_and> : bit_and {
   template <typename Left, typename Right>
   decltype(auto) operator() (Left&& l, Right&& r) const
   {
      return std::forward<Left>(l &= std::forward<Right>(r));
   }
};

template <>
struct assign<bit_or> : bit_or {
   template <typename Left, typename Right>
   decltype(auto) operator() (Left&& l, Right&& r) const
   {
      return std::forward<Left>(l |= std::forward<Right>(r));
   }
};

template <>
struct assign<bit_xor> : bit_xor {
   template <typename Left, typename Right>
   decltype(auto) operator() (Left&& l, Right&& r) const
   {
      return std::forward<Left>(l ^= std::forward<Right>(r));
   }
};

template <>
struct assign<lshift> : lshift {
   template <typename Left, typename Right>
   decltype(auto) operator() (Left&& l, Right&& r) const
   {
      return std::forward<Left>(l <<= std::forward<Right>(r));
   }
};

template <>
struct assign<rshift> : rshift {
   template <typename Left, typename Right>
   decltype(auto) operator() (Left&& l, Right&& r) const
   {
      return std::forward<Left>(l >>= std::forward<Right>(r));
   }
};


/// the flavor of the operation with an implicit default right operand as in sparse containers
/// by default, such a flavor is undefined, which implies that the operation would deliver a default value
template <typename Operation>
struct partial_left : std::false_type {};

/// the flavor of the operation with an implicit default left operand as in sparse containers
/// by default, such a flavor is undefined, which implies that the operation would deliver a default value
template <typename Operation>
struct partial_right : std::false_type {};

/// temporarily cast the given operation to its partial flavor
template <typename Operation>
const partial_left<Operation>& partial_left_flavor_of(const Operation& op)
{
   return static_cast<const partial_left<Operation>&>(op);
}

template <typename Operation>
const partial_right<Operation>& partial_right_flavor_of(const Operation& op)
{
   return static_cast<const partial_right<Operation>&>(op);
}

template <>
struct partial_left<add> : add {
   template <typename Left, typename Iterator2>
   Left&& operator() (Left&& l, const Iterator2&) const
   {
      return std::forward<Left>(l);
   }
};

template <>
struct partial_right<add> : add {
   template <typename Iterator1, typename Right>
   Right&& operator() (const Iterator1&, Right&& r) const
   {
      return std::forward<Right>(r);
   }
};

template <>
struct partial_left<sub> : sub {
   template <typename Left, typename Iterator2>
   Left&& operator() (Left&& l, const Iterator2&) const
   {
      return std::forward<Left>(l);
   }
};

template <>
struct partial_right<sub> : sub {
   template <typename Iterator1, typename Right>
   decltype(auto) operator() (const Iterator1&, Right&& r) const
   {
      return -std::forward<Right>(r);
   }
};

template <>
struct partial_left<bit_or> : bit_or {
   template <typename Left, typename Iterator2>
   Left&& operator() (Left&& l, const Iterator2&) const
   {
      return std::forward<Left>(l);
   }
};

template <>
struct partial_right<bit_or> : bit_or {
   template <typename Iterator1, typename Right>
   Right&& operator() (const Iterator1&, Right&& r) const
   {
      return std::forward<Right>(r);
   }
};

template <>
struct partial_left<bit_xor> : bit_xor {
   template <typename Left, typename Iterator2>
   Left&& operator() (Left&& l, const Iterator2&) const
   {
      return std::forward<Left>(l);
   }
};

template <>
struct partial_right<bit_xor> : bit_xor {
   template <typename Iterator1, typename Right>
   Right&& operator() (const Iterator1&, Right&& r) const
   {
      return std::forward<Right>(r);
   }
};

/// tell whether the given operation has well-defined partial flavors dealing with implicit default operands
template <typename Operation>
using is_partially_defined
   = bool_not<mlist_or<is_derived_from<partial_left<Operation>, std::false_type>,
                       is_derived_from<partial_right<Operation>, std::false_type>>>;

}

}

namespace pm {
namespace cleanOperations = polymake::cleanOperations;
}


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
