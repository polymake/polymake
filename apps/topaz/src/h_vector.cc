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
#include "polymake/topaz/complex_tools.h"

namespace polymake { namespace topaz {

Array<Int> h_vector(const Array<Int> &F)
{
   const Int dim = F.size()-1;
   Array<Int> h_vector(dim+2);
   h_vector[0] = 1;
   for (Int k = 1; k <= dim+1; ++k) { 
      Integer h_k(Integer::pow(Integer(-1),k) * Integer::binom(dim+1,k));
      for (Int j = 1; j <= k; ++j) 
         h_k += Integer::pow(Integer(-1),k-j) * Integer::binom(dim+1-j, k-j) * F[j-1];
      h_vector[k] = Int(h_k);
   }
   return h_vector;
}

Function4perl(&h_vector,"h_vector");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
