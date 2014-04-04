/* Copyright (c) 1997-2014
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

#ifndef POLYMAKE_NUMERICAL_FUNCTIONS_H
#define POLYMAKE_NUMERICAL_FUNCTIONS_H

#include "polymake/GenericStruct.h"
#include <cstdlib>

namespace pm {

inline bool abs_equal(int x, int y) { return x==y || -x==y; }
inline bool abs_equal(long x, long y) { return x==y || -x==y; }
inline bool abs_equal(long x, int y) { return x==y || -x==y; }
inline bool abs_equal(int x, long y) { return x==y || -x==y; }
inline bool abs_equal(double x, double y) { return x==y || x==-y; }
inline bool abs_equal(double x, long y) { return x==y || x==-y; }
inline bool abs_equal(long x, double y) { return x==y || -x==y; }
inline bool abs_equal(double x, int y) { return x==y || x==-y; }
inline bool abs_equal(int x, double y) { return x==y || -x==y; }

inline
long gcd(long a, long b)
{
   if (a<0) a=-a;
   if (b<0) b=-b;
   if (a==0) return b;
   if (b==0) return a;
   if (a==1 || b==1) return 1;

   int k=0;
   while (!((a|b)&1)) {
      a>>=1;  b>>=1;  k++;
   }

   if (a&1)
      while (!(b&1)) b>>=1;
   else
      while (!(a&1)) a>>=1;

   long d=a-b;
   while (d) {
      while (!(d&1)) d>>=1;
      if (d>0)
         a=d;
      else
         b=-d;
      d=a-b;
   }

   return a<<k;
}

inline
long lcm(long a, long b)
{
   return (a/gcd(a,b))*b;
}

/// result of the extended gcd calculation for two numbers (a, b)
template <typename T>
struct ExtGCD : GenericStruct< ExtGCD<T> > {

   DeclSTRUCT( DeclTemplFIELD(g, T)  // g = gcd(a, b)
               DeclTemplFIELD(p, T)  // g == p * a + q * b
               DeclTemplFIELD(q, T)
               DeclTemplFIELD(k1, T) // a == k1 * g
               DeclTemplFIELD(k2, T) // b == k2 * g
             );
};

inline
ExtGCD<long> ext_gcd(long a, long b)
{
   ExtGCD<long> res;

   if (a==0) {
      res.g=b;
      res.p=res.q=res.k2=1;
      res.k1=0;

   } else if (b==0) {
      res.g=a;
      res.p=res.q=res.k1=1;
      res.k2=0;

   } else {
      // We are looking for a 2x2 matrix U s.t. U*(a,b)=(0,g) or (g,0)
      // We start with a unit matrix.
      const bool sw=a<b;
      if (sw) std::swap(a,b);

      long U[2][2]={ { (a<0 ? (a=-a, -1L) : 1L), 0 },
                     { 0, (b<0 ? (b=-b, -1L) : 1L) } };
      long k;

      for (;;) {
         k = a/b;
         // multiply U from left with { {1, -k}, {0, 1} }
         U[0][0] -= k * U[1][0];
         U[0][1] -= k * U[1][1];
         if ((a -= k*b) == 0) {
            res.g=b;
            res.p=U[1][sw];   res.q=U[1][1-sw];
            res.k2=U[0][sw];  res.k1=U[0][1-sw];
            negate(sw ? res.k2 : res.k1);
            break;
         }

         k = b/a;
         // multiply U from left with { {1, 0}, {-k, 1} }
         U[1][0] -= k * U[0][0];
         U[1][1] -= k * U[0][1];
         if ((b -= k*a) == 0) {
            res.g=a;
            res.p=U[0][sw];   res.q=U[0][1-sw];
            res.k2=U[1][sw];  res.k1=U[1][1-sw];
            negate(sw ? res.k1 : res.k2);
            break;
         }
      }
   }

   return res;
}

/// result of integer division of two numbers (a,b)
template <typename T>
struct Div : GenericStruct< Div<T> > {

   DeclSTRUCT( DeclTemplFIELD(quot, T)  // quotient=a/b
               DeclTemplFIELD(rem, T)   // remainder=a-quot*b
             );
};

inline
Div<long> div(long a, long b)
{
   Div<long> res;
   res.quot=a/b;
   res.rem=a%b;
   return res;
}

inline
long div_exact(long a, long b)
{
   return a/b;
}

#if defined(__GNUC__)
inline int log2_floor(unsigned int x)
{
   return sizeof(x)*8 -1 - __builtin_clz(x);
}

inline int log2_floor(unsigned long x)
{
   return sizeof(x)*8 -1 - __builtin_clzl(x);
}

inline int log2_ceil(unsigned int x)
{
   return x > 1 ? log2_floor(x-1)+1 : 0;
}

inline int log2_ceil(unsigned long x)
{
   return x > 1 ? log2_floor(x-1)+1 : 0;
}

#else // !GCC

inline int log2_round(unsigned long x, int round)
{
   if (x<=1UL) return 0;

   int step=(sizeof(unsigned long)*8)/2, log2=step;
   unsigned long pow2=1UL<<step;

   while (x != pow2) {
      if (x < pow2) {
         if (!(step >>= 1)) return log2-1+round;
         pow2 >>= step;
         log2 -= step;
      } else {
         if (!(step >>= 1)) return log2+round;
         pow2 <<= step;
         log2 += step;
      }
   }
   return log2;
}

inline int log2_floor(unsigned long x) { return log2_round(x,0); }
inline int log2_ceil(unsigned long x) { return log2_round(x,1); }

#endif

inline int log2_floor(int x)  { return log2_floor((unsigned int)x); }
inline int log2_floor(long x) { return log2_floor((unsigned long)x); }
inline int log2_ceil(int x)   { return log2_ceil((unsigned int)x); }
inline int log2_ceil(long x)  { return log2_ceil((unsigned long)x); }

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
}

#endif // POLYMAKE_NUMERICAL_FUNCTIONS_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
