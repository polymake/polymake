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
#include <string>
#include "polymake/topaz/complex_tools.h"
#include "polymake/topaz/connected_sum.h"

namespace polymake { namespace topaz {

BigObject surface(Int g)
{
   const bool oriented = g >= 0;
   g = abs(g);

   BigObject p("SimplicialComplex");
   std::ostringstream description;
   description << (oriented ? "Oriented " : "Non-oriented ") << "surface of genus "<< g << ".\n";
   p.set_description() << description.str();

   if (g==0) {    // return 2-sphere
      Array<Set<Int>> C(4, all_subsets_less_1(sequence(0,4)).begin());

      p.take("FACETS") << C;
      p.take("DIM") << 2;
      p.take("MANIFOLD") << 1;
      p.take("CLOSED_PSEUDO_MANIFOLD") << 1;
      p.take("ORIENTED_PSEUDO_MANIFOLD") << 1;
      p.take("SPHERE") << 1;

   } else {
      // produce g toruses / projective planes and compute their connected sum
      std::list<Set<Int>> C;
      if (oriented) {
         const Array<Set<Int>> T = torus_facets();
         copy_range(entire(T), std::back_inserter(C));
         for (Int i = 0; i < g-1; ++i) {
            C=connected_sum(C,T);
         }
      } else {
         const Array<Set<Int>> PJP = real_projective_plane_facets();
         copy_range(entire(PJP), std::back_inserter(C));
         for (Int i = 0; i<g-1; ++i) {
            C=connected_sum(C,PJP);
         }
      }

      p.take("FACETS") << C;
      p.take("DIM") << 2;
      p.take("MANIFOLD") << 1;
      p.take("CLOSED_PSEUDO_MANIFOLD") << 1;
      p.take("ORIENTED_PSEUDO_MANIFOLD") << oriented;
   }
   return p;
}

UserFunction4perl("# @category Producing from scratch\n"
                  "# Produce a __surface of genus //g//__. For //g// >= 0\n"
                  "# the client produces an orientable surface, otherwise\n"
                  "# it produces a non-orientable one.\n"
                  "# @param Int g genus"
                  "# @return SimplicialComplex",
                  &surface, "surface($)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
