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

#include "polymake/GenericStruct.h"
#include "polymake/internal/type_manip.h"

namespace pm {

template <typename TNum1, typename TNum2> inline
typename std::enable_if<(std::is_arithmetic<pure_type_t<TNum1>>::value &&
                         std::is_arithmetic<pure_type_t<TNum2>>::value), bool>::type
abs_equal(TNum1 x, TNum2 y)
{
   return x==y || -x==y;
}

long gcd(long a, long b) noexcept;

inline
long lcm(long a, long b)
{
   return (a/gcd(a,b))*b;
}

inline
long gcd(long a, int b)
{
   return gcd(a, long(b));
}
inline
long gcd(int a, long b)
{
   return gcd(long(a), b);
}
inline
long gcd(int a, int b)
{
   return gcd(long(a), long(b));
}
inline
long lcm(long a, int b)
{
   return lcm(a, long(b));
}
inline
long lcm(int a, long b)
{
   return lcm(long(a), b);
}
inline
long lcm(int a, int b)
{
   return lcm(long(a), long(b));
}

/// result of the extended gcd calculation for two numbers (a, b)
template <typename T>
struct ExtGCD : GenericStruct< ExtGCD<T> > {

   DeclSTRUCT( DeclTemplFIELD(g, T)  // g = gcd(a, b)
               DeclTemplFIELD(p, T)  // g == p*a + q*b
               DeclTemplFIELD(q, T)
               DeclTemplFIELD(k1, T) // a == k1*g
               DeclTemplFIELD(k2, T) // b == k2*g
             );
};

ExtGCD<long> ext_gcd(long a, long b) noexcept;

/// result of integer division of two numbers (a,b)
template <typename T>
struct Div : GenericStruct< Div<T> > {

   DeclSTRUCT( DeclTemplFIELD(quot, T)  // quotient=a/b
               DeclTemplFIELD(rem, T)   // remainder=a-quot*b
             );
};

inline
Div<long> div(long a, long b) noexcept
{
   Div<long> result;
   result.quot=a/b;
   result.rem=a%b;
   return result;
}

inline
long div_exact(long a, long b) noexcept
{
   return a/b;
}

#if defined(__GNUC__) || defined(__clang__)
inline
int log2_floor(unsigned int x) noexcept
{
   return int(sizeof(x))*8-1-__builtin_clz(x);
}

inline
int log2_floor(unsigned long x) noexcept
{
   return int(sizeof(x))*8-1-__builtin_clzl(x);
}

inline
int log2_ceil(unsigned int x) noexcept
{
   return x > 1 ? log2_floor(x-1)+1 : 0;
}

inline
int log2_ceil(unsigned long x) noexcept
{
   return x > 1 ? log2_floor(x-1)+1 : 0;
}

#else // neither GCC nor clang

int log2_round(unsigned long x, int round) noexcept;

inline int log2_floor(unsigned long x) { return log2_round(x, 0); }
inline int log2_ceil(unsigned long x) { return log2_round(x, 1); }

#endif

inline int log2_floor(int x)  { return log2_floor((unsigned int)x); }
inline int log2_floor(long x) { return log2_floor((unsigned long)x); }
inline int log2_ceil(int x)   { return log2_ceil((unsigned int)x); }
inline int log2_ceil(long x)  { return log2_ceil((unsigned long)x); }

template <typename T>
T pow_impl(T base, T odd, long exp)
{
   while (exp > 1) {
      if (exp%2 == 0) {
         base = base * base;
         exp = exp/2;
      } else {
         odd = base * odd;
         base = base * base;
         exp = (exp-1)/2;
      }
   }
   return base * odd;
}

template <typename T>
T pow(const T& base, long exp,
      std::enable_if_t<std::is_same<typename object_traits<T>::generic_tag, is_scalar>::value, std::nullptr_t> = nullptr)
{
   auto one = one_value<T>();
   if (exp < 0) {
      return pow_impl<T>(one/base,one,abs(exp));
   } else if (exp == 0) {
      return one;
   }
   return pow_impl<T>(base,one,exp);
}

namespace operations {

template <typename Base, typename Exp, typename=std::enable_if_t<std::is_same<Exp, long>::value>>
struct pow_impl {
   typedef Base first_argument_type;
   typedef Exp second_argument_type;
   typedef const Base result_type;

   result_type operator() (typename function_argument<Base>::type a, Exp b) const
   {
      return pm::pow(a,b);
   }
};

template <typename Base, typename Exp>
struct pow : pow_impl<Base,Exp> {};

}

}

namespace polymake {
   using pm::gcd;
   using pm::ext_gcd;
   using pm::ExtGCD;
   using pm::lcm;
   using pm::div;
   using pm::Div;
   using pm::div_exact;
   using pm::log2_floor;
   using pm::log2_ceil;
   using pm::abs_equal;
   using pm::pow;

   namespace operations {
      using pm::operations::pow;
   }
}


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
