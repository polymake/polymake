/* Copyright (c) 1997-2021
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
#include "polymake/FacetList.h"
#include "polymake/Set.h"

namespace polymake { namespace topaz {

typedef Array<Set<Int>> face_list;

void faces_to_facets(BigObject p, const face_list& F_in)
{
   FacetList F;
   Set<Int> V;

   for (auto f_it=entire(F_in); !f_it.at_end(); ++f_it) {
      // provide vertex ordering
      Set<Int> facet;
      accumulate_in(entire(*f_it), operations::add(), facet);

      // add facet to vertex set
      V+=facet;
      F.insertMax(facet);
   }

   // checking numbering
   const bool renumber= !V.empty() && (V.front()!=0 || V.back()+1!=V.size());

   if (renumber) F.squeeze();
   if (F.empty()) {
      p.take("FACETS") << Array<Set<Int>>(1, V);
   } else {
      p.take("FACETS") << F;
   }

   p.take("VERTEX_INDICES") << V;
   p.take("N_VERTICES") << V.size();
}

Function4perl(&faces_to_facets, "faces_to_facets(SimplicialComplex $)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
