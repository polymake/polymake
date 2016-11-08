/* Copyright (c) 1997-2015
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

#include "polymake/topaz/merge_vertices.h"

namespace polymake { namespace topaz {

hash_map<int,int> merge_vertices(Array<std::string>& L1, const Array<std::string>& L2)
{
   const int n_vert1 = L1.size();
   const int n_vert2 = L2.size();
   hash_map<int,int> M(n_vert2);

   // compute vertex maps
   hash_map<std::string,int> map(n_vert1);
   int count = 0;
    
   for (Entire< Array<std::string> >::const_iterator l=entire(L1);
        !l.at_end(); ++l, ++count)
      map[*l] = count;
    
   L1.resize(n_vert1 + n_vert2);
   int diff = n_vert1;
   count = 0;
   for (Entire< Array<std::string> >::const_iterator l=entire(L2);
        !l.at_end(); ++l, ++count)
      if (map.find(*l) != map.end()) {         // label equal to *l found in L1
         M[count] = map[*l];
         --diff;
      } else {
         M[count] = diff + count;
         L1[diff + count] = *l;
      }
   L1.resize(diff + count);

   return M;
}
  
void merge_disjoint_vertices (Array<std::string>& L1, const Array<std::string>& L2)
{
   const int n_vert1 = L1.size(); 
   const int n_vert2 = L2.size(); 
    
   // adjust labels
   L1.resize(n_vert1 + n_vert2);
    
   for (int v=0; v<n_vert1; ++v)
      L1[v] = L1[v] + "_1";
    
   for (int v=0; v<n_vert2; ++v)
      L1[n_vert1 + v] = L2[v]+"_2";
}

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
