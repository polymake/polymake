/* Copyright (c) 1997-2020
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
#include "polymake/Array.h"
#include "polymake/PowerSet.h"
#include "polymake/FacetList.h"
#include "polymake/topaz/lawler.h"

namespace polymake { namespace topaz {
  
Array<Set<Int>> lawler(const Array<Set<Int>>& F, const Int n_vertices)
{
   sequence Vertices(0, n_vertices);

   // C stores the candidates for minimal non-faces
   // insert every single element in the complement of the first facet

   FacetList C=all_subsets_of_1(Vertices-F[0]);

   // incrementally insert the facets into the complex (skip the first facet)
   auto f = entire(F);
   for (++f; !f.at_end(); ++f) {
      Set<Int> V = Vertices-*f;    // compute the complement of the current facet
      FacetList Cnew(n_vertices);
      // for every element in C
      for (auto cit=entire(C); !cit.at_end(); ++cit) {

         // and every vertex in the complement of the facet
         for (auto vit=entire(V); !vit.at_end(); ++vit)
            Cnew.replaceMin(*cit + *vit);
      }
      C = Cnew;
   }

   return Array<Set<Int>>(C);
}

Function4perl(&lawler, "lawler_minimal_non_faces(Array<Set<Int>>, $)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
