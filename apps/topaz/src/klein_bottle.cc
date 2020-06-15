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
#include "polymake/topaz/connected_sum.h"
#include "polymake/topaz/complex_tools.h"

namespace polymake { namespace topaz {
  
BigObject klein_bottle()
{
   const Array<Set<Int>> PJP = real_projective_plane_facets();
   const std::list<Set<Int>> C = connected_sum(PJP,PJP);
   BigObject p("SimplicialComplex",
               "FACETS", C,
               "DIM", 2,
               "MANIFOLD", true,
               "CLOSED_PSEUDO_MANIFOLD", true,
               "ORIENTED_PSEUDO_MANIFOLD", false);
   p.set_description() << "The Klein bottle.\n";
   return p;
}

UserFunction4perl("# @category Producing from scratch\n"
                  "# The Klein bottle.\n"
                  "# @return SimplicialComplex",
                  &klein_bottle, "klein_bottle()");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
