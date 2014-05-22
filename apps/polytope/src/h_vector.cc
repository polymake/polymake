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

#include "polymake/client.h"
#include "polymake/Vector.h"
#include "polymake/Integer.h"
#include <algorithm>

namespace polymake { namespace polytope {

// Compute the h-vector of a simplicial or simple polytope from the f-vector.

void h_vector(perl::Object p, bool simplicial)
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

void f_vector(perl::Object p, bool simplicial)
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

Function4perl(&h_vector, "h_vector");
Function4perl(&f_vector, "f_vector");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
