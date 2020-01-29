/* Copyright (c) 1997-2020
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

#ifndef POLYMAKE_INTERNAL_CONVERTERS_BASIC_DEFS_H
#define POLYMAKE_INTERNAL_CONVERTERS_BASIC_DEFS_H

#include "polymake/internal/nothing.h"

namespace pm {

// special case of trivial no-conversion
template <typename T>
class trivial_conv {
public:
   typedef T argument_type;
   typedef typename function_argument<T>::type result_type;

   const T& operator() (const T& x) const { return x; }
   T&& operator() (T&& x) const { return std::move(x); }
};

template <typename From, typename To, bool enabled=std::is_constructible<To, From>::value>
class conv_default : public nothing {};

template <typename From, typename To>
class conv_default<From, To, true> {
public:
   typedef pure_type_t<From> argument_type;
   typedef To result_type;

   To operator() (const From& x) const { return static_cast<To>(x); }
};

/// Explicit type converter.
template <typename From, typename To>
class conv : public conv_default<From, To> {};

template <typename T>
class conv<T, T> : public trivial_conv<T> {};

template <typename From, typename To>
using is_explicitly_convertible_to
   = bool_not<is_derived_from<conv<From, To>, nothing>>;

template <typename From, typename To>
using is_only_explicitly_convertible_to
   = bool_constant<is_explicitly_convertible_to<From, To>::value && !std::is_convertible<From, To>::value>;

template <typename Source, typename Target>
struct can_initialize
   : bool_constant< isomorphic_types<Source, Target>::value &&
                    ( std::is_convertible<Source, Target>::value || is_explicitly_convertible_to<Source, Target>::value) > {};

template <typename T>
struct can_initialize<std::nullptr_t, T*>
   : std::true_type {};

template <typename Source, typename Target>
struct can_upgrade
   : bool_constant< isomorphic_types<Source, Target>::value && std::is_convertible<Source, Target>::value > {};

} // end namespace pm

#endif // POLYMAKE_INTERNAL_CONVERTERS_BASIC_DEFS_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
