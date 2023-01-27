/* Copyright (c) 1997-2023
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

#include "polymake/internal/converters_basic_defs.h"
#include <iostream>
#include <limits>

namespace polymake { namespace topaz {

enum class GF2_old : uint8_t { zero = 0, one = 1 };

inline GF2_old operator+(GF2_old x) { return x; }
inline GF2_old operator-(GF2_old x) { return x; }

inline GF2_old operator+ (GF2_old a, GF2_old b) { return GF2_old(int(a)^int(b)); }
inline GF2_old operator- (GF2_old a, GF2_old b) { return GF2_old(int(a)^int(b)); }
inline GF2_old operator* (GF2_old a, GF2_old b) { return GF2_old(int(a)&int(b)); }
inline GF2_old operator/ (GF2_old a, GF2_old b) { return GF2_old(int(a)/int(b)); }

inline GF2_old& operator+= (GF2_old& a, GF2_old b) { return a = a+b; }
inline GF2_old& operator-= (GF2_old& a, GF2_old b) { return a = a-b; }
inline GF2_old& operator*= (GF2_old& a, GF2_old b) { return a = a*b; }
inline GF2_old& operator/= (GF2_old& a, GF2_old b) { return a = a/b; }

template <typename Traits>
std::basic_istream<char, Traits>&
operator>> (std::basic_istream<char, Traits>& is, GF2_old& a)
{
   int val;
   is >> val;
   if (val == 0)
      a = GF2_old::zero;
   else if (val == 1)
      a = GF2_old::one;
   else
      is.setstate(std::ios::failbit);
   return is;
}

template <typename Traits>
std::basic_ostream<char, Traits>&
operator<< (std::basic_ostream<char, Traits>& os, GF2_old a)
{
   return os << int(a);
}

template <typename T, bool is_integer = std::numeric_limits<T>::is_integer>
class conv_to_GF2_old;

template <typename T>
class conv_to_GF2_old<T, true> {
public:
   typedef T argument_type;
   typedef GF2_old result_type;
   result_type operator() (const T& x) const { return result_type(x%2); }
};

} } // end namespace polymake::topaz

namespace std {

template <>
struct numeric_limits<polymake::topaz::GF2_old> : numeric_limits<unsigned int> {
   static polymake::topaz::GF2_old min() throw() { return polymake::topaz::GF2_old::zero; }
   static polymake::topaz::GF2_old max() throw() { return polymake::topaz::GF2_old::one; }
   static const int digits = 1;
   static const int digits10 = 1;
};

}

namespace pm {

template <typename T>
class conv<T, polymake::topaz::GF2_old> : public polymake::topaz::conv_to_GF2_old<T> { };

template <>
class conv<polymake::topaz::GF2_old, polymake::topaz::GF2_old> : public trivial_conv<polymake::topaz::GF2_old> { } ;

template<>
struct algebraic_traits<polymake::topaz::GF2_old> {
   typedef polymake::topaz::GF2_old field_type;
};

template <>
struct spec_object_traits<polymake::topaz::GF2_old>
   : spec_object_traits<is_scalar> {

   static
   bool is_zero(const polymake::topaz::GF2_old& a)
   {
      return a == polymake::topaz::GF2_old::zero;
   }

   static
   bool is_one(const polymake::topaz::GF2_old& a)
   {
      return a == polymake::topaz::GF2_old::one;
   }

   static const polymake::topaz::GF2_old& zero() { static const auto z = polymake::topaz::GF2_old::zero; return z; }

   static const polymake::topaz::GF2_old& one() { static const auto o = polymake::topaz::GF2_old::one; return o; }
};

}


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
