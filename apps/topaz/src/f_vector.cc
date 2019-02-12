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
#include "polymake/topaz/complex_tools.h"

namespace polymake { namespace topaz {
  
Array<int> f_vector(const Array< Set<int> > &C,const int dim,const bool is_pure)
{
   Array<int> f_vector(dim+1);
   for (int k=0; k<=dim; ++k) {
      const PowerSet<int>& skeleton = k_skeleton(C,k);
      if (is_pure) {
         f_vector[k] = skeleton.size();
      } else {
         int f_k=0;
         for (auto f=entire(skeleton); !f.at_end(); ++f)
            if (f->size()==k+1)
               ++f_k;

         f_vector[k] = f_k;
      }
   }
   return f_vector;
}

Function4perl(&f_vector,"f_vector");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
