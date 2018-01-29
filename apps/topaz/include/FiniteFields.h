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

#ifndef POLYMAKE_TOPAZ_FINITE_FIELDS_H
#define POLYMAKE_TOPAZ_FINITE_FIELDS_H

#include "polymake/internal/converters_basic_defs.h"
#include <iostream>
#include <limits>

namespace polymake { namespace topaz {

enum GF2 { GF2_zero=0, GF2_one=1 };

inline GF2 operator+(GF2 x) { return x; }
inline GF2 operator-(GF2 x) { return x; }

inline GF2 operator+ (GF2 a, GF2 b) { return GF2(int(a)^int(b)); }
inline GF2 operator- (GF2 a, GF2 b) { return GF2(int(a)^int(b)); }
inline GF2 operator* (GF2 a, GF2 b) { return GF2(int(a)&int(b)); }
inline GF2 operator/ (GF2 a, GF2 b) { return GF2(int(a)/int(b)); }

inline GF2& operator+= (GF2& a, GF2 b) { reinterpret_cast<int&>(a)^=int(b); return a; }
inline GF2& operator-= (GF2& a, GF2 b) { reinterpret_cast<int&>(a)^=int(b); return a; }
inline GF2& operator*= (GF2& a, GF2 b) { reinterpret_cast<int&>(a)&=int(b); return a; }
inline GF2& operator/= (GF2& a, GF2 b) { reinterpret_cast<int&>(a)/=int(b); return a; }

template <typename Traits> inline
std::basic_istream<char, Traits>&
operator>> (std::basic_istream<char, Traits>& is, GF2& a)
{
   return is >> reinterpret_cast<int&>(a);
}

template <typename Traits> inline
std::basic_ostream<char, Traits>&
operator<< (std::basic_ostream<char, Traits>& os, GF2 a)
{
   return os << int(a);
}

template <typename T, bool _is_integer=std::numeric_limits<T>::is_integer>
class conv_to_GF2;

template <typename T>
class conv_to_GF2<T, true> {
public:
   typedef T argument_type;
   typedef GF2 result_type;
   result_type operator() (typename pm::function_argument<T>::type x) const { return result_type(x%2); }
};

} } // end namespace polymake::topaz

namespace std {

template <>
struct numeric_limits<polymake::topaz::GF2> : numeric_limits<unsigned int> {
   static polymake::topaz::GF2 min() throw() { return polymake::topaz::GF2_zero; }
   static polymake::topaz::GF2 max() throw() { return polymake::topaz::GF2_one; }
   static const int digits=1;
   static const int digits10=1;
};

}

namespace pm {

template <typename T>
class conv<T, polymake::topaz::GF2> : public polymake::topaz::conv_to_GF2<T> { };

template <>
class conv<polymake::topaz::GF2, polymake::topaz::GF2> : public trivial_conv<polymake::topaz::GF2> { } ;

template<>
struct algebraic_traits<polymake::topaz::GF2> {
   typedef polymake::topaz::GF2 field_type;
};

template <>
struct choose_generic_object_traits<polymake::topaz::GF2, false, false> :
      spec_object_traits< polymake::topaz::GF2 > {
   typedef void generic_type;
   typedef is_scalar generic_tag;
   typedef polymake::topaz::GF2 persistent_type;

   static
   bool is_zero(const polymake::topaz::GF2& a)
   {
      return a == polymake::topaz::GF2_zero;
   }

   static
   bool is_one(const polymake::topaz::GF2& a)
   {
      return a == polymake::topaz::GF2_one;
   }

   static const persistent_type& zero() 
   {
      static polymake::topaz::GF2 zero = polymake::topaz::GF2_zero;
      return zero;
   }

   static const persistent_type& one() 
   {
      static polymake::topaz::GF2 one = polymake::topaz::GF2_one;
      return one;
   }
};

}

#endif // POLYMAKE_TOPAZ_FINITE_FIELDS_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
