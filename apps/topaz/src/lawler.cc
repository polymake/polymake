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

#include "polymake/client.h"
#include "polymake/Array.h"
#include "polymake/PowerSet.h"
#include "polymake/FacetList.h"

namespace polymake { namespace topaz {
  
Array<Set<int>> lawler(const Array<Set<int> > F, const int n_vertices )
{
   sequence Vertices(0,n_vertices);

   // C stores the candidates for minimal non-faces
   // insert every single element in the complement of the first facet

   FacetList C=all_subsets_of_1(Vertices-F[0]);

   // incrementally insert the facets into the complex (skip the first facet)
   Entire< Array< Set<int> > >::const_iterator f=entire(F);
   for (++f; !f.at_end(); ++f) {
      Set<int> V = Vertices-*f;    // compute the complement of the current facet
      FacetList Cnew(n_vertices);
      // for every element in C
      for (Entire<FacetList>::const_iterator cit=entire(C); !cit.at_end(); ++cit) {

         // and every vertex in the complement of the facet
         for (Entire< Set<int> >::const_iterator vit=entire(V); !vit.at_end(); ++vit)
            Cnew.replaceMin(*cit + *vit);
      }
      C = Cnew;
   }

   return Array<Set<int>>(C);
}

Function4perl(&lawler, "lawler_minimal_non_faces(Array<Set<Int>>, $)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
