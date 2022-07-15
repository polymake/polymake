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
#include "polymake/topaz/complex_tools.h"

namespace polymake { namespace topaz {

Int is_manifold_client(BigObject p)
{
   const Array<Set<Int>> C = p.give("FACETS");
   const Int d = p.give("DIM");
   const Int n_vertices = p.give("N_VERTICES");

   Int answer;
   switch (d) {
   case 1:
      answer=is_manifold(C, n_vertices, int_constant<1>());
      break;
   case 2:
      answer=is_manifold(C, n_vertices, int_constant<2>());
      break;
   case 3:
      answer=is_manifold(C, n_vertices, int_constant<3>());
      break;
   default:
      answer=-1; // don't know
   }

   return answer;
}

Function4perl(&is_manifold_client, "is_manifold(SimplicialComplex)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
