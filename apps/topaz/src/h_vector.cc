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
#include "polymake/topaz/complex_tools.h"

namespace polymake { namespace topaz {

Array<int> h_vector(const Array<int> &F)
{
   const int dim = F.size()-1;
   Array<int> h_vector(dim+2);
   h_vector[0]=1;
   for (int k=1; k<=dim+1; ++k) { 
      Integer h_k(Integer::pow(Integer(-1),k) * Integer::binom(dim+1,k));
      for (int j=1; j<=k; ++j) 
         h_k = h_k + Integer::pow(Integer(-1),k-j) * Integer::binom(dim+1-j, k-j) * Integer(F[j-1]);
      h_vector[k]=h_k.to_int();
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
