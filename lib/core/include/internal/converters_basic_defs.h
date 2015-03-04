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

#ifndef POLYMAKE_INTERNAL_CONVERTERS_BASIC_DEFS_H
#define POLYMAKE_INTERNAL_CONVERTERS_BASIC_DEFS_H

#include "polymake/internal/nothing.h"

namespace pm {

/** Explicit type converter.
 *  The primary template expresses the case of a *lacking* conversion.
 */
template <typename From, typename To>
class conv : public nothing {};

template <typename From, typename To>
struct explicitly_convertible_to
{
   static const bool value = !identical<From, To>::value && !convertible_to<From, To>::value && !derived_from<conv<From, To>, nothing>::value;
};

// special case of trivial no-conversion
template <typename T>
class trivial_conv {
public:
   typedef T argument_type;
   typedef typename function_argument<T>::type result_type;

   typename function_argument<T>::type operator() (typename function_argument<T>::type x) const { return x; }
};

template <typename T>
class conv<T, T> : public trivial_conv<T> {};

template <typename From, typename To>
class conv_by_cast {
public:
   typedef typename attrib<From>::minus_const_ref argument_type;
   typedef To result_type;

   To operator() (typename function_argument<From>::type x) const { return To(x); }
};

} // end namespace pm

#endif // POLYMAKE_INTERNAL_CONVERTERS_BASIC_DEFS_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
