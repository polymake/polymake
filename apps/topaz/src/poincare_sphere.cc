/* Copyright (c) 1997-2023
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
  
Array<Set<Int>> poincare_sphere_facets()
{
   return {
      {0,1,3,8}, {0,1,3,14}, {0,1,5,13}, {0,1,5,14}, {0,1,8,13}, {0,2,3,11}, {0,2,3,14}, {0,2,6,9}, {0,2,6,11},
      {0,2,9,14}, {0,3,8,11}, {0,4,5,12}, {0,4,5,13}, {0,4,7,10}, {0,4,7,12}, {0,4,10,13}, {0,5,12,14}, {0,6,7,9},
      {0,6,7,10}, {0,6,10,11}, {0,7,9,12}, {0,8,10,11}, {0,8,10,13}, {0,9,12,14}, {1,2,4,9}, {1,2,4,10}, {1,2,6,9},
      {1,2,6,12}, {1,2,10,12}, {1,3,8,12}, {1,3,10,12}, {1,3,10,14}, {1,4,7,10}, {1,4,7,11}, {1,4,9,11}, {1,5,9,11},
      {1,5,9,13}, {1,5,11,14}, {1,6,8,12}, {1,6,8,13}, {1,6,9,13}, {1,7,10,14}, {1,7,11,14}, {2,3,4,13}, {2,3,4,14},
      {2,3,11,13}, {2,4,9,14}, {2,4,10,13}, {2,6,11,12}, {2,10,12,13}, {2,11,12,13}, {3,4,5,6}, {3,4,5,13}, {3,4,6,14},
      {3,5,6,10}, {3,5,9,10}, {3,5,9,13}, {3,6,10,14}, {3,7,8,11}, {3,7,8,12}, {3,7,9,12}, {3,7,9,13}, {3,7,11,13},
      {3,9,10,12}, {4,5,6,12}, {4,6,8,12}, {4,6,8,14}, {4,7,8,11}, {4,7,8,12}, {4,8,9,11}, {4,8,9,14}, {5,6,10,11},
      {5,6,11,12}, {5,9,10,11}, {5,11,12,14}, {6,7,9,13}, {6,7,10,14}, {6,7,13,14}, {6,8,13,14}, {7,11,13,14}, {8,9,10,11},
      {8,9,10,15}, {8,9,14,15}, {8,10,13,15}, {8,13,14,15}, {9,10,12,15}, {9,12,14,15}, {10,12,13,15}, {11,12,13,14}, {12,13,14,15}
   };
}

BigObject poincare_sphere()
{
   BigObject p("SimplicialComplex",
               "FACETS", poincare_sphere_facets(),
               "DIM", 3,
               "MANIFOLD", true,
               "CLOSED_PSEUDO_MANIFOLD", true,
               "ORIENTED_PSEUDO_MANIFOLD", true);
   p.set_description() << "Poincare homology 3-sphere on 16 vertices.\n";
   return p;
}


UserFunction4perl("# @category Producing from scratch\n"
		  "# The 16-vertex triangulation of the Poincaré homology 3-sphere by Björner and Lutz,\n"
                  "# Experimental Mathematics, Vol. 9 (2000), No. 2.\n"
		  "# @return SimplicialComplex"
                  "# @example Print the face numbers."
                  "# > print poincare_sphere()->F_VECTOR;"
                  "# | 16 106 180 90",
		  &poincare_sphere, "poincare_sphere()");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
