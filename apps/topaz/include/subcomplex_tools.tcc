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

namespace polymake { namespace topaz {

template <typename Complex>
PowerSet<int> k_skeleton(const Complex& C, const int k)
{
   typedef typename Complex::value_type Facet;
   PowerSet<int> SK;
  
   for (typename Entire<Complex>::const_iterator f_it=entire(C); !f_it.at_end(); ++f_it) {
      // the k skeleton are all faces of dimension <= k.
      if (f_it->size()<=k) {
         // facets are either equal or none contains the other.
         SK+=*f_it;
         continue;
      }

      // the k skeleton a facet are all subsets of size k+1
      Subsets_of_k< const Facet& > enumerator(*f_it, k+1);     
      for (typename Entire< Subsets_of_k< const Facet& > >::iterator s_it=entire(enumerator); !s_it.at_end(); ++s_it) {
        // facets are either equal or none contains the other.
        SK+=*s_it;
      }
   }
   if (SK.empty())  // the empty complex is represented as a single empty set
      SK += Set<int>();
   
   return SK;
}

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
