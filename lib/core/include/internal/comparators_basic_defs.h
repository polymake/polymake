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

#ifndef POLYMAKE_INTERNAL_COMPARATORS_BASIC_DEFS_H
#define POLYMAKE_INTERNAL_COMPARATORS_BASIC_DEFS_H

#include "polymake/internal/operations_basic_defs.h"

#include <limits>
#include <cstdlib>
#include <cmath>
#include <string>
#include <functional>

namespace pm {

enum cmp_value { cmp_lt=-1, cmp_eq=0, cmp_gt=1, cmp_ne=cmp_gt };

template <typename T>
constexpr int sign_impl(T x, std::true_type)
{
   return x<0 ? -1 : x>0;
}

template <typename T>
constexpr int sign_impl(T x, std::false_type)
{
   return x!=0;
}

template <typename T>
constexpr
typename std::enable_if<std::is_arithmetic<T>::value, int>::type
sign(T x)
{
   return sign_impl(x, bool_constant<std::numeric_limits<T>::is_signed>());
}

template <typename TPrimitive, typename T>
constexpr
typename std::enable_if<std::is_arithmetic<T>::value, TPrimitive>::type
max_value_as(mlist<T>)
{
   return static_cast<TPrimitive>(std::numeric_limits<T>::max());
}

template <typename TPrimitive, typename T>
constexpr
typename std::enable_if<std::is_arithmetic<T>::value, TPrimitive>::type
min_value_as(mlist<T>)
{
   return static_cast<TPrimitive>(std::numeric_limits<T>::min());
}

template <typename T, bool is_max> struct extremal {};
template <typename T> struct maximal : extremal<T, true> {};
template <typename T> struct minimal : extremal<T, false> {};

namespace operations {

template <typename T1, typename T2=T1, bool ordered=is_ordered<T1>::value>
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

template <typename T1, typename T2>
struct cmp_basic<T1, T2, false> {
   typedef T1 first_argument_type;
   typedef T2 second_argument_type;
   typedef cmp_value result_type;

   template <typename Left, typename Right>
   cmp_value operator() (const Left& a, const Right& b) const
   {
      return a==b ? cmp_eq : cmp_ne;
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

template <typename T1, typename T2=T1, bool ordered=(is_ordered<T1>::value && is_ordered<T2>::value)>
struct cmp_extremal_if_ordered : cmp_extremal {};

template <typename T1, typename T2>
struct cmp_extremal_if_ordered<T1, T2, false> {
   // some impossible combination...
   cmp_value operator() (void**, void**) const;
};

template <typename T, bool use_zero_test=has_zero_value<T>::value>
struct cmp_partial_opaque {
   template <typename Left, typename Iterator2>
   cmp_value operator() (partial_left, const Left&, const Iterator2&) const
   {
      return cmp_gt;
   }

   template <typename Iterator1, typename Right>
   cmp_value operator() (partial_right, const Iterator1&, const Right&) const
   {
      return is_ordered<Right>::value ? cmp_lt : cmp_ne;
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

template <typename T1, typename T2=T1,
          bool is_signed=(std::numeric_limits<T1>::is_signed && std::numeric_limits<T2>::is_signed)>
struct cmp_scalar : cmp_extremal, cmp_partial_scalar {

   typedef T1 first_argument_type;
   typedef T2 second_argument_type;
   typedef cmp_value result_type;

   using cmp_extremal::operator();
   using cmp_partial_scalar::operator();

   template <typename Left, typename Right>
   typename std::enable_if<(std::numeric_limits<Left>::is_integer && std::numeric_limits<Right>::is_signed &&
                            std::numeric_limits<Left>::is_integer && std::numeric_limits<Right>::is_signed), cmp_value>::type
   operator() (const Left& a, const Right& b) const
   {
      return cmp_value(sign(a-b));
   }

   template <typename Left, typename Right>
   typename std::enable_if<!(std::numeric_limits<Left>::is_integer && std::numeric_limits<Right>::is_signed &&
                             std::numeric_limits<Left>::is_integer && std::numeric_limits<Right>::is_signed), cmp_value>::type
   operator() (const Left& a, const Right& b) const
   {
      return cmp_basic<Left, Right, true>()(a, b);
   }
};

template <typename T1, typename T2>
struct cmp_scalar<T1, T2, false> : cmp_extremal, cmp_basic<T1, T2, true> {

   using cmp_extremal::operator();
   using cmp_basic<T1, T2, true>::operator();

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

template <typename T1, typename T2=T1>
struct cmp_opaque : cmp_extremal_if_ordered<T1>, cmp_basic<T1, T2>, cmp_partial_opaque<T1> {
   using cmp_extremal_if_ordered<T1>::operator();
   using cmp_basic<T1, T2>::operator();
   using cmp_partial_opaque<T1>::operator();
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

template <typename T1, typename T2> inline
T1& assign_max(T1& max, const T2& x) { if (max<x) max=x; return max; }

template <typename T1, typename T2> inline
T1& assign_min(T1& min, const T2& x) { if (min>x) min=x; return min; }

template <typename T1, typename T2, typename T3> inline
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
int isinf(double x) noexcept
{
   return std::isinf(x) ? (x>0)*2-1 : 0;
}

constexpr int isinf(long) { return 0; }
constexpr int isinf(int) { return 0; }

template <typename T, bool _is_max>
struct spec_object_traits< extremal<T,_is_max> > : spec_object_traits<T> {};

template <typename T>
struct spec_object_traits< maximal<T> > : spec_object_traits<T> {};

template <typename T>
struct spec_object_traits< minimal<T> > : spec_object_traits<T> {};

} // end namespace pm

#endif // POLYMAKE_INTERNAL_COMPARATORS_BASIC_DEFS_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
