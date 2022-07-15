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

#include "polymake/client.h"
#include "polymake/Integer.h"
#include "polymake/Array.h"
#include "polymake/Vector.h"
#include "polymake/Matrix.h"
#include "polymake/list"
#include "polymake/Fibonacci.h"

namespace polymake { namespace polytope {

namespace {

Integer calc_p(Int n, Int j)
{
   return Integer::binom(n,j) - Integer::binom(n,j-1);
}
      
Integer calc_b(Int deg, Int v, Int n, const Array<Int>& Fib)
{
   std::list<Int> l;
   Int curl = 0;
   Int rd = deg;
   Int rv = v;
   while (rd > 1)
      if (rv >= Fib[rd-1]) {    // we have a 'd'
         if (curl%2 != 0) // the previous exponent of the previous 'c' was not even...
            return 0;   // ...so b(v,n) is 0
         rv -= Fib[rd-1];
         rd -= 2;
         l.push_back(curl/2);
         curl = 0;
      } else {          // it's a 'c'
         rd -= 1;
         ++curl;
      }

   if (rd == 1) ++curl;

   if (curl%2 != 0) return 0;
   curl /= 2;

   Int r = 0;
   Int i = deg/2;
   Integer res = calc_p(n-2*i+2*curl, curl);
   while (!l.empty()) {
      curl = l.front();
      l.pop_front();
      res *= calc_p(2*curl, curl);
      ++r;
   }

   return (i-r)%2==0 ? res : -res;
}

bool ends_with_c(Int deg, Int v, const Array<Int>& Fib) // ...is there a faster way to tell?
{
   bool endc = true;
   Int rd = deg;
   Int rv = v;
   while (rd > 1)
      if (rv >= Fib[rd-1]) {    // we have a 'd'
         rv -= Fib[rd-1];
         rd -= 2;
         endc = false;
      } else {          // it's a 'c'
         rd -= 1;
         endc = true;
      }
   if (rd == 1)         // there's a lonely 'c' and the end
      endc = true;

   return endc;
}
} // end anonymous namespace

/*  Calculate the toric g-vector and the (generalised) h-vector from the cd-index of a polytope.
 *  See
 *      Richard Ehrenborg, Lifting inequalities for polytopes,
 *      Adv.Math. 193 (2005), 205-222, section 4
 *
 *  @author Axel Werner
 */

void toric_g_vector(BigObject p)
{
   Int d = p.give ("COMBINATORIAL_DIM");
   Vector<Integer> cd = p.give("CD_INDEX_COEFFICIENTS");

   // precalculate the fibonacci numbers
   const Array<Int> Fib(d+1, fibonacci_numbers());

   // holds the g-vector
   Vector<Integer> gvec(d/2+1);
   gvec[0] = 1;

   for (Int i = 1; i <= d/2; ++i) {
      // calculate g_i in this loop
      Int deg = 2*i;
      Int max_cdmon = Fib[deg];
      Int bigv = 0;     // count the full monomial separately
      Int degfill = d-deg;
      for (Int v = 0; v < max_cdmon; ++v) {
         Integer moncoeff = calc_b(deg,v,d,Fib);
         gvec[i] += (moncoeff*cd[bigv]);
         if (ends_with_c(deg,v,Fib)) // increase the full-monomial-counter
            bigv += Fib[degfill+1];
         else
            bigv += Fib[degfill];
      }
   }

   Vector<Integer> hvec (d+1);
   hvec[0] = hvec[d] = 1;
   for (Int i = 1; i <= d/2; ++i)
      hvec[d-i] = hvec[i] = gvec[i] + hvec[i-1];

   p.take("G_VECTOR") << gvec;
   p.take("H_VECTOR") << hvec;
}

Function4perl(&toric_g_vector, "toric_g_vector");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
