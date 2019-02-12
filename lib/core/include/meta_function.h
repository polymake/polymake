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

#ifndef POLYMAKE_META_FUNCTION_H
#define POLYMAKE_META_FUNCTION_H

#include <type_traits>

namespace polymake {

template <bool Value>
using bool_constant = std::integral_constant<bool, Value>;

template <int Value>
using int_constant = std::integral_constant<int, Value>;

template <char Value>
using char_constant = std::integral_constant<char, Value>;

template <typename T>
using bool_not
   = std::integral_constant<bool, !T::value>;

/// Canonicalized compile-time constant
template <typename T>
using mvalue_of
   = std::integral_constant<typename T::value_type, T::value>;

/// Operations on compile-time constants

template <typename Const1, typename Const2>
using mis_equal
   = bool_constant<(Const1::value == Const2::value)>;

template <typename Const1, typename Const2>
using mis_less
   = bool_constant<(Const1::value < Const2::value)>;

template <typename Const1, typename Const2>
using mis_greater
   = bool_constant<(Const1::value > Const2::value)>;

template <typename Const1, typename Const2>
using mminimum
   = std::conditional<mis_less<Const1, Const2>::value, Const1, Const2>;

template <typename Const1, typename Const2>
using mmaximum
   = std::conditional<mis_greater<Const1, Const2>::value, Const1, Const2>;

template <typename Const1, typename Const2>
using madd
   = std::integral_constant<decltype(Const1::value + Const2::value), Const1::value + Const2::value>;

template <typename Const1, typename Const2>
using msubtract
   = std::integral_constant<decltype(Const1::value - Const2::value), Const1::value - Const2::value>;


/// Choose the type occurring in the first satisifed clause.
/// Every clause but the last one must be an instance of std::enable_if.
/// The last clause may be a single type with serves as an 'otherwise' case.
/// If there is no such fallback, the resulting type is undefined.
template <typename... TClauses>
struct mselect;

template <typename T, typename T2, typename... Tail>
struct mselect<std::enable_if<true, T>, T2, Tail...> {
   using type = T;
};

template <typename T, typename T2, typename... Tail>
struct mselect<std::enable_if<false, T>, T2, Tail...>
   : mselect<T2, Tail...> {};

// final conditional clause and no 'otherwise' case
template <bool cond, typename T>
struct mselect<std::enable_if<cond, T>>
   : std::enable_if<cond, T> {};
 
// 'otherwise' case
template <typename T>
struct mselect<T> {
   using type = T;
};

/// Operation on a pair of types: selects the first one

template <typename T1, typename T2>
struct mproject1st {
  using type = T1;
};

/// Operation on a pair of types: selects the first one unless it is void
template <typename T1, typename T2>
struct mprefer1st {
  using type = T1;
};

template <typename T2>
struct mprefer1st<void, T2> {
  using type = T2;
};

/// Operation on a pair of types: selects the second one
template <typename T1, typename T2>
struct mproject2nd {
  using type = T2;
};

/// Operation on a pair of types: selects the second one unless it is void
template <typename T1, typename T2>
struct mprefer2nd {
  using type = T2;
};

template <typename T1>
struct mprefer2nd<T1, void> {
  using type = T1;
};

/// container for arbitrary many types
template <typename... T>
struct mlist {};

/// Check whether a type is an instance of a given class template
template <typename T, template <typename...> class Template>
struct is_instance_of
   : std::false_type {
   using params = void;
};

template <typename... T, template <typename...> class Template>
struct is_instance_of<Template<T...>, Template>
   : std::true_type {
   using params = mlist<T...>;
};

/// wrapper for a meta-function
/// allows to pass meta-functions as elements of (tagged) meta-lists
template <template <typename...> class Func>
struct meta_function {
   template <typename... T>
   using func = Func<T...>;
};


// as long as we don't have C++17 void_t
template <typename... T>
using accept_valid_type = void;

template <template <typename...> class T>
using accept_valid_template = void;


/// Safely evaluate a unary boolean meta-function
/// Return false for void input
template <typename T, template <typename> class Func>
struct msafely_eval_boolean
  : bool_constant<Func<T>::value> {};

template <template <typename> class Func>
struct msafely_eval_boolean<void, Func>
  : std::false_type {};


/// Safely evaluate a meta-function.
/// If its result is undefined, deliver the fallback value.
template <typename Func, typename Fallback, typename IsDefined = void>
struct msafely_eval {
   using type = Fallback;
};

template <typename Func, typename Fallback>
struct msafely_eval<Func, Fallback, accept_valid_type<typename Func::type>>
   : Func {};

/// Wrappers for arbitrary meta-functions

/// Unary meta-function always returns the same result
template <typename Const>
struct mbind_const {
   template <typename>
   struct func {
      using type = Const;
   };
};

/// Reduce a binary meta-function to a unary one by fixing the first argument to a given value
template <template <typename, typename> class BinaryFunc, typename Const>
struct mbind1st {
   template <typename Arg>
   using func = BinaryFunc<Const, Arg>;
};

/// Reduce a binary meta-function to a unary one by fixing the second argument to a given value
template <template <typename, typename> class BinaryFunc, typename Const>
struct mbind2nd {
   template <typename Arg>
   using func = BinaryFunc<Arg, Const>;
};

/// Swap arguments passed to a binary meta-function
template <template <typename, typename> class BinaryFunc>
struct mswap_args {
   template <typename Arg1, typename Arg2>
   using func = BinaryFunc<Arg2, Arg1>;
};

/// Negate the result of a unary boolean meta-function
template <template <typename> class UnaryFunc>
struct mnegate_unary {
   template <typename Arg>
   using func = bool_constant<!UnaryFunc<Arg>::value>;
};

/// Negate the result of a binary boolean meta-function
template <template <typename, typename> class BinaryFunc>
struct mnegate_binary {
   template <typename Arg1, typename Arg2>
   using func = bool_constant<!BinaryFunc<Arg1, Arg2>::value>;
};

}

#endif // POLYMAKE_META_FUNCTION_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
