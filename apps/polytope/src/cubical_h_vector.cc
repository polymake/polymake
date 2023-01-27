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

#include "polymake/client.h"
#include "polymake/Integer.h"
#include "polymake/Vector.h"

namespace polymake { namespace polytope {
  
/* Compute the cubical h-vector of a cubical or cocubical polytope
 * from the f-vector as defined in "A New Cubical $h$-Vector" by Ron
 * M. Adin in 1995.
 */

void cubical_h_vector(BigObject p, bool cubical)
{
   Vector<Integer> f = p.give("F_VECTOR");
   if (!cubical) std::reverse(f.begin(),f.end());

   const Int d = f.size();
   Vector<Integer> h(d+1);
   Vector<Integer>::iterator h_k=h.begin();
    
   *h_k = Integer::pow(2,d-1);
   ++h_k;
   for (Int k = 0, startsign = 1;  k <= d-1;  ++k, ++h_k, startsign = -startsign) {
      for (Int i = 0, sign = startsign;  i <= k;  ++i, sign = -sign)
         *h_k += sign * Integer::binom(d-1-i,d-1-k) * (Integer::pow(2,i)) * f[i];
      *h_k -= h_k[-1];
   }
    
   p.take("CUBICAL_H_VECTOR") << h;
}

Function4perl(&cubical_h_vector, "cubical_h_vector");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
