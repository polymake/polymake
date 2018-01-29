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

#include "polymake/client.h"
#include "polymake/Vector.h"
#include "polymake/Integer.h"
#include <algorithm>

namespace polymake { namespace polytope {

// Compute the (dual) h-vector of a simplicial or simple polytope from the f-vector.
void h_from_f_vector(perl::Object p, bool simplicial)
{
   Vector<Integer> f=p.give("F_VECTOR");
   if (!simplicial) std::reverse(f.begin(), f.end());

   const int d=f.size();
   Vector<Integer> h(d+1);
   Vector<Integer>::iterator h_k=h.begin();
   for (int k=0, startsign=1;  k<=d;  ++k, ++h_k, startsign=-startsign) {
      *h_k = startsign * Integer::binom(d,d-k);
      for (int i=1, sign=-startsign;  i<=k;  ++i, sign=-sign)
         *h_k += sign * Integer::binom(d-i,d-k) * f[i-1];
   }

   if (simplicial)
      p.take("H_VECTOR") << h;
   else
      p.take("DUAL_H_VECTOR") << h;
}

// Inverse of the above
void f_from_h_vector(perl::Object p, bool simplicial)
{
   Vector<Integer> h;
   if (simplicial) {
      Vector<Integer> h_read=p.give("H_VECTOR");
      h=h_read;
   } else {
      Vector<Integer> h_read=p.give("DUAL_H_VECTOR");
      h=h_read;
   }

   const int d=h.size()-1;
   Vector<Integer> f(d);
   for (int k=0; k<d; ++k) {
      Integer f_k=0;
      for (int i=k; i<=d; ++i)
         f_k+=Integer::binom(i,k)*h[i];
      if (simplicial)
         f[d-1-k]=f_k;
      else
         f[k]=f_k;
   }
   
   p.take("F_VECTOR") << f;
}

// Compute the h-vector from the g-vector of simplicial polytope
void h_from_g_vector(perl::Object p)
{
   const Vector<Integer> g=p.give("G_VECTOR");
   const int d=p.give("COMBINATORIAL_DIM");

   Vector<Integer> h(d+1);
   Integer s(0);
   for (int k=0; k<=d/2; ++k) {
      s += g[k];
      h[d-k] = h[k] = s;
   }

   p.take("H_VECTOR") << h;
}

// Inverse of the above
void g_from_h_vector(perl::Object p)
{
   const Vector<Integer> h=p.give("H_VECTOR");
   const int d=h.dim()-1;

   Vector<Integer> g((d+2)/2);
   g[0]=1;
   for (int k=1; k<(d+2)/2; ++k) {
      g[k] = h[k]-h[k-1];
   }

   p.take("G_VECTOR") << g;
}

Function4perl(&h_from_f_vector, "h_from_f_vector");
Function4perl(&f_from_h_vector, "f_from_h_vector");
Function4perl(&h_from_g_vector, "h_from_g_vector");
Function4perl(&g_from_h_vector, "g_from_h_vector");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
