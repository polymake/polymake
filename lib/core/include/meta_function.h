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

#ifndef POLYMAKE_META_FUNCTION_H
#define POLYMAKE_META_FUNCTION_H

#include <type_traits>

namespace polymake {

template <bool TValue>
using bool_constant = std::integral_constant<bool, TValue>;

template <int TValue>
using int_constant = std::integral_constant<int, TValue>;

template <char TValue>
using char_constant = std::integral_constant<char, TValue>;

/// Choose the type occurring in the first satisifed clause.
/// Every clause but the last one must be an instance of std::enable_if.
/// The last clause may be a single type with serves as an 'otherwise' case.
/// If there is no such fallback, the resulting type is undefined.
template <typename... TClauses>
struct mselect;

template <typename T>
struct mselect<std::enable_if<true, T>> {
   typedef T type;
};

template <typename T, typename T2, typename... TTail>
struct mselect<std::enable_if<true, T>, T2, TTail...> {
   typedef T type;
};

template <typename T, typename T2, typename... TTail>
struct mselect<std::enable_if<false, T>, T2, TTail...>
   : mselect<T2, TTail...> {};

template <typename T>
struct mselect<std::enable_if<false, T>> {};

template <typename T>
struct mselect<T> {
   typedef T type;
};

/// Operation on a pair of types: selects the first one

template <typename T1, typename T2>
struct mproject1st {
  typedef T1 type;
};

/// Operation on a pair of types: selects the first one unless it is void
template <typename T1, typename T2>
struct mprefer1st {
  typedef T1 type;
};

template <typename T2>
struct mprefer1st<void, T2> {
  typedef T2 type;
};

/// Operation on a pair of types: selects the second one
template <typename T1, typename T2>
struct mproject2nd {
  typedef T2 type;
};

/// Operation on a pair of types: selects the second one unless it is void
template <typename T1, typename T2>
struct mprefer2nd {
  typedef T2 type;
};

template <typename T1>
struct mprefer2nd<T1, void> {
  typedef T1 type;
};

/// container for arbitrary many types
template <typename... T>
struct mlist {};

/// Check whether a type is an instance of a given class template
template <typename T, template <typename...> class Template>
struct is_instance_of
   : std::false_type {
   typedef mlist<> params;
};

template <typename... T, template <typename...> class Template>
struct is_instance_of<Template<T...>, Template>
   : std::true_type {
   typedef mlist<T...> params;
};

/// wrapper for a meta-function
/// allows to pass meta-functions as elements of (tagged) meta-lists
template <template <typename...> class TFunction>
struct meta_function {
   template <typename T> struct apply;

   template <typename... T>
   struct apply<mlist<T...>>
      : TFunction<T...> {};
};


/// Safely evaluate a meta-function.
/// If its result is undefined, deliver the fallback value.
template <typename TFunction, typename TFallback, typename TDefined=void>
struct mevaluate {
   typedef TFallback type;
};

template <typename TFunction, typename TFallback>
struct mevaluate<TFunction, TFallback, typename mproject2nd<typename TFunction::type, void>::type>
   : TFunction {};

}

#endif // POLYMAKE_META_FUNCTION_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
