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

#include "polymake/internal/operations_basic_defs.h"

#include <limits>
#include <cstdlib>
#include <cmath>
#include <string>
#include <functional>

namespace pm {

enum cmp_value { cmp_lt=-1, cmp_eq=0, cmp_gt=1, cmp_ne=cmp_gt };

template <typename T>
constexpr Int sign_impl(T x, std::true_type)
{
   return x < 0 ? -1 : x > 0;
}

template <typename T>
constexpr Int sign_impl(T x, std::false_type)
{
   return x != 0;
}

template <typename T>
constexpr
std::enable_if_t<std::is_arithmetic<T>::value, Int>
sign(T x)
{
   return sign_impl(x, bool_constant<std::numeric_limits<T>::is_signed>());
}

template <typename TPrimitive, typename T>
constexpr
std::enable_if_t<std::is_arithmetic<T>::value, TPrimitive>
max_value_as(mlist<T>)
{
   return static_cast<TPrimitive>(std::numeric_limits<T>::max());
}

template <typename TPrimitive, typename T>
constexpr
std::enable_if_t<std::is_arithmetic<T>::value, TPrimitive>
min_value_as(mlist<T>)
{
   return static_cast<TPrimitive>(std::numeric_limits<T>::min());
}

template <typename T, bool is_max> struct extremal {};
template <typename T> struct maximal : extremal<T, true> {};
template <typename T> struct minimal : extremal<T, false> {};

namespace operations {

template <typename T1, typename T2>
struct cmp_basic {
   typedef T1 first_argument_type;
   typedef T2 second_argument_type;
   typedef cmp_value result_type;

   template <typename Left, typename Right>
   cmp_value operator() (const Left& a, const Right& b) const
   {
      return a<b ? cmp_lt : cmp_value(a>b);
   }
};

struct cmp_extremal {
   template <typename T, bool _is_max_l, bool _is_max_r>
   cmp_value operator() (const extremal<T,_is_max_l>&, const extremal<T,_is_max_r>&) const
   {
      return _is_max_l == _is_max_r ? cmp_eq : _is_max_l ? cmp_gt : cmp_lt;
   }

   template <typename T, bool _is_max_l>
   cmp_value operator() (const extremal<T,_is_max_l>&, const T&) const
   {
      return _is_max_l ? cmp_gt : cmp_lt;
   }

   template <typename T, bool _is_max_r>
   cmp_value operator() (const T&, const extremal<T,_is_max_r>&) const
   {
      return _is_max_r ? cmp_lt : cmp_gt;
   }
};

template <typename T, bool use_zero_test = has_zero_value<T>::value>
struct cmp_partial_opaque {
   template <typename Left, typename Iterator2>
   cmp_value operator() (partial_left, const Left&, const Iterator2&) const
   {
      return cmp_gt;
   }

   template <typename Iterator1, typename Right>
   cmp_value operator() (partial_right, const Iterator1&, const Right&) const
   {
      return cmp_lt;
   }
};

template <typename T>
struct cmp_partial_opaque<T, true> {
   template <typename Left, typename Iterator>
   cmp_value operator() (partial_left, const Left& x, const Iterator&) const
   {
      return is_zero(x) ? cmp_eq : cmp_gt;
   }

   template <typename Iterator, typename Right>
   cmp_value operator() (partial_right, const Iterator&, const Right& x) const
   {
      return is_zero(x) ? cmp_eq : cmp_lt;
   }
};

struct cmp_partial_scalar {
   template <typename Left, typename Iterator>
   cmp_value operator() (partial_left, const Left& a, const Iterator&) const
   {
      return cmp_value(sign(a));
   }
   template <typename Iterator, typename Right>
   cmp_value operator() (partial_right, const Iterator&, const Right& b) const
   {
      return cmp_value(-sign(b));
   }
};

template <typename T1, typename T2=T1, typename enabled=void>
struct cmp_scalar { };

template <typename T1, typename T2>
struct cmp_scalar<T1, T2, typename std::enable_if<std::numeric_limits<T1>::is_signed && std::numeric_limits<T2>::is_signed &&
                                                  are_less_greater_comparable<T1, T2>::value>::type>
   : cmp_extremal
   , cmp_partial_scalar {

   typedef T1 first_argument_type;
   typedef T2 second_argument_type;
   typedef cmp_value result_type;

   using cmp_extremal::operator();
   using cmp_partial_scalar::operator();

   template <typename Left, typename Right>
   typename std::enable_if<(std::numeric_limits<Left>::is_integer && std::numeric_limits<Left>::is_signed &&
                            std::numeric_limits<Right>::is_integer && std::numeric_limits<Right>::is_signed), cmp_value>::type
   operator() (const Left& a, const Right& b) const
   {
      return cmp_value(sign(a-b));
   }

   template <typename Left, typename Right>
   typename std::enable_if<!(std::numeric_limits<Left>::is_integer && std::numeric_limits<Left>::is_signed &&
                             std::numeric_limits<Right>::is_integer && std::numeric_limits<Right>::is_signed), cmp_value>::type
   operator() (const Left& a, const Right& b) const
   {
      return cmp_basic<Left, Right>()(a, b);
   }
};

template <typename T1, typename T2>
struct cmp_scalar<T1, T2, typename std::enable_if<!(std::numeric_limits<T1>::is_signed && std::numeric_limits<T2>::is_signed) &&
                                                  are_less_greater_comparable<T1, T2>::value>::type>
   : cmp_extremal
   , cmp_basic<T1, T2> {

   using cmp_extremal::operator();
   using cmp_basic<T1, T2>::operator();

   template <typename Left, typename Iterator>
   cmp_value operator() (partial_left, const Left& a, const Iterator&) const
   {
      return is_zero(a) ? cmp_eq : cmp_gt;
   }
   template <typename Iterator, typename Right>
   cmp_value operator() (partial_right, const Iterator&, const Right& b) const
   {
      return is_zero(b) ? cmp_eq : cmp_lt;
   }
};

template <typename T1, typename T2=T1, typename enabled=void>
struct cmp_unordered_impl { };

template <typename T1, typename T2>
struct cmp_unordered_impl<T1, T2, typename std::enable_if<are_comparable<T1, T2>::value>::type> {
   typedef T1 first_argument_type;
   typedef T2 second_argument_type;
   typedef cmp_value result_type;

   static const bool partially_defined = has_zero_value<T1>::value && has_zero_value<T2>::value;

   template <typename Left, typename Right>
   cmp_value operator()(const Left& l, const Right& r) const
   {
      return l==r ? cmp_eq : cmp_ne;
   }

   template <typename Left, typename Iterator>
   std::enable_if_t<has_zero_value<Left>::value, cmp_value>
   operator() (partial_left, const Left& a, const Iterator&) const
   {
      return is_zero(a) ? cmp_eq : cmp_ne;
   }

   template <typename Iterator, typename Right>
   std::enable_if_t<has_zero_value<Right>::value, cmp_value>
   operator() (partial_right, const Iterator&, const Right& b) const
   {
      return is_zero(b) ? cmp_eq : cmp_ne;
   }
};

template <typename T, typename enabled=void>
struct cmp_opaque { };

template <typename T>
struct cmp_opaque<T, typename std::enable_if<is_less_greater_comparable<T>::value>::type>
   : cmp_extremal
   , cmp_basic<T, T>
   , cmp_partial_opaque<T> {
   using cmp_extremal::operator();
   using cmp_basic<T, T>::operator();
   using cmp_partial_opaque<T>::operator();
};

template <typename Char, typename Traits, typename Alloc>
struct cmp_partial_opaque<std::basic_string<Char, Traits, Alloc>, false> {

   template <typename Left, typename Iterator>
   cmp_value operator() (partial_left, const Left& a, const Iterator&) const
   {
      return a.empty() ? cmp_eq : cmp_gt;
   }
   template <typename Iterator, typename Right>
   cmp_value operator() (partial_right, const Iterator&, const Right& b) const
   {
      return b.empty() ? cmp_eq : cmp_lt;
   }
};

template <typename T1, typename T2=T1>
struct cmp_scalar_with_leeway : cmp_extremal {

   typedef T1 first_argument_type;
   typedef T2 second_argument_type;
   typedef cmp_value result_type;

   using cmp_extremal::operator();

   template <typename Left, typename Right>
   cmp_value operator()(const Left& a, const Right& b) const
   {
      return !is_zero(a-b) ? cmp_scalar<Left,Right>()(a, b) : cmp_eq;
   }

   template <typename Left, typename Iterator2>
   cmp_value operator()(partial_left, const Left& a, const Iterator2& b) const
   {
      return !is_zero(a) ? cmp_partial_scalar()(partial_left(), a, b) : cmp_eq;
   }

   template <typename Right, typename Iterator1>
   cmp_value operator()(partial_right, const Iterator1& a, const Right& b) const
   {
      return !is_zero(b) ? cmp_partial_scalar()(partial_right(), a, b) : cmp_eq;
   }
};

template <typename T>
struct cmp_pointer {
   typedef T* first_argument_type;
   typedef T* second_argument_type;
   typedef cmp_value result_type;

   cmp_value operator()(T* a, T* b) const
   {
      return cmp_scalar<long, long>()(long(a), long(b));
   }
};

} // end namespace operations

template <typename T1, typename T2>
T1& assign_max(T1& max, const T2& x) { if (max<x) max=x; return max; }

template <typename T1, typename T2>
T1& assign_min(T1& min, const T2& x) { if (min>x) min=x; return min; }

template <typename T1, typename T2, typename T3>
void assign_min_max(T1& min, T2& max, const T3& x)
{
   if (min>x) min=x; else if (max<x) max=x;
}

template <typename T, typename Tag=typename object_traits<T>::generic_tag>
struct hash_func;

template <typename T>
struct hash_func<T, is_scalar> : public std::hash<T> {};

template <typename Char, typename Traits, typename Alloc>
struct hash_func<std::basic_string<Char, Traits, Alloc>, is_opaque> : public std::hash< std::basic_string<Char, Traits, Alloc> > {};

template <typename T>
struct hash_func<T*, is_not_object> {
   size_t operator() (T* ptr) const { return size_t(ptr); }
};

using std::abs;
using std::isfinite;

/// return the sign of the inifinite value, or 0 if the value is finite
/// std::isinf returns bool nowadays which is insufficient for efficient operations
inline
Int isinf(double x) noexcept
{
   return std::isinf(x) ? (x>0)*2-1 : 0;
}

constexpr Int isinf(long) { return 0; }

template <typename T, bool is_max>
struct spec_object_traits< extremal<T, is_max> > : spec_object_traits<T> {};

template <typename T>
struct spec_object_traits< maximal<T> > : spec_object_traits<T> {};

template <typename T>
struct spec_object_traits< minimal<T> > : spec_object_traits<T> {};

template <typename T1, typename T2, typename Comparator, typename=cmp_value>
struct are_comparable_via : std::false_type {};

template <typename T1, typename T2, typename Comparator>
struct are_comparable_via<T1, T2, Comparator, decltype(std::declval<Comparator>()(std::declval<const T1&>(), std::declval<const T2&>()))> : std::true_type {};

} // end namespace pm


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
