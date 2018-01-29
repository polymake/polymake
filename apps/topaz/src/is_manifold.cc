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

int is_manifold_client(perl::Object p)
{
   const Array< Set<int> > C = p.give("FACETS");
   const int d = p.give("DIM");
   const int n_vertices = p.give("N_VERTICES");

   int answer;
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
