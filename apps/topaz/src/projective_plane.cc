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
#include "polymake/topaz/complex_tools.h"

namespace polymake { namespace topaz {
  
Array<Set<Int>> real_projective_plane_facets()
{
   return { { 0, 1, 4 },
            { 0, 1, 5 },
            { 0, 2, 3 },
            { 0, 2, 4 },
            { 0, 3, 5 },
            { 1, 2, 3 },
            { 1, 2, 5 },
            { 1, 3, 4 },
            { 2, 4, 5 },
            { 3, 4, 5 } };
}

BigObject real_projective_plane()
{
   BigObject p("SimplicialComplex",
               "FACETS", real_projective_plane_facets(),
               "DIM", 2,
               "MANIFOLD", true,
               "CLOSED_PSEUDO_MANIFOLD", true,
               "ORIENTED_PSEUDO_MANIFOLD", false);
   p.set_description() << "Real projective plane on six vertices.\n";
   return p;
}

Array<Set<Int>> complex_projective_plane_facets()
{
   return { {0, 1, 2, 3, 4},
            {0, 1, 2, 3, 6},
            {0, 1, 2, 4, 7},
            {0, 1, 2, 6, 7},
            {0, 1, 3, 4, 5},
            {0, 1, 3, 5, 6},
            {0, 1, 4, 5, 7},
            {0, 1, 5, 6, 8},
            {0, 1, 5, 7, 8},
            {0, 1, 6, 7, 8},
            {0, 2, 3, 4, 8},
            {0, 2, 3, 6, 7},
            {0, 2, 3, 7, 8},
            {0, 2, 4, 5, 7},
            {0, 2, 4, 5, 8},
            {0, 2, 5, 7, 8},
            {0, 3, 4, 5, 6},
            {0, 3, 4, 6, 8},
            {0, 3, 6, 7, 8},
            {0, 4, 5, 6, 8},
            {1, 2, 3, 4, 8},
            {1, 2, 3, 5, 6},
            {1, 2, 3, 5, 8},
            {1, 2, 4, 6, 7},
            {1, 2, 4, 6, 8},
            {1, 2, 5, 6, 8},
            {1, 3, 4, 5, 7},
            {1, 3, 4, 7, 8},
            {1, 3, 5, 7, 8},
            {1, 4, 6, 7, 8},
            {2, 3, 5, 6, 7},
            {2, 3, 5, 7, 8},
            {2, 4, 5, 6, 7},
            {2, 4, 5, 6, 8},
            {3, 4, 5, 6, 7},
            {3, 4, 6, 7, 8} };
}

BigObject complex_projective_plane()
{
   BigObject p("SimplicialComplex",
               "FACETS", complex_projective_plane_facets(),
               "DIM", 4,
               "MANIFOLD", true,
               "CLOSED_PSEUDO_MANIFOLD", true,
               "ORIENTED_PSEUDO_MANIFOLD", true);
   p.set_description() << "Complex projective plane on nine vertices.\n";
   return p;
}

UserFunction4perl("# @category Producing from scratch\n"
		  "# The real projective plane with its unique minimal triangulation on six vertices.\n"
		  "# @return SimplicialComplex",
		  &real_projective_plane, "real_projective_plane()");

UserFunction4perl("# @category Producing from scratch\n"
		  "# The complex projective plane with the vertex-minimal triangulation by Kühnel and Brehm.\n"
		  "# @return SimplicialComplex"
        "# @example Construct the complex projective plane, store it in the variable $p2c, and print its homology group."
        "# > $p2c = complex_projective_plane();"
        "# > print $p2c->HOMOLOGY;"
        "# | ({} 0)"
        "# | ({} 0)"
        "# | ({} 1)"
        "# | ({} 0)"
        "# | ({} 1)",
		  &complex_projective_plane, "complex_projective_plane()");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
