/* Copyright (c) 1997-2014
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
#include "polymake/Matrix.h"

namespace polymake { namespace topaz {
  
Array< Set<int> > torus_facets()
{
   static const int Torus[][3]= { {0, 1, 2},
                                  {0, 1, 3},
                                  {0, 3, 4},
                                  {1, 2, 5},
                                  {0, 4, 5},
                                  {1, 4, 5},
                                  {2, 3, 4},
                                  {2, 3, 5},
                                  {0, 5, 6},
                                  {1, 4, 6},
                                  {3, 5, 6},
                                  {2, 4, 6},
                                  {1, 3, 6},
                                  {0, 2, 6} };

   return Array< Set<int> >(Torus);
}

perl::Object torus()
{
   perl::Object p("GeometricSimplicialComplex<Rational>");
   p.set_description() << "The Császár torus. Geometric realization by Frank Lutz, Electronic Geometry Model No. 2001.02.069\n";

   static const int Coordinates[][3]= { {3, -3, 0},
                                        {-3, 3, 0},
                                        {-3, -3, 1},
                                        {3, 3, 1},
                                        {-1, -2, 3},
                                        {1, 2, 3},
                                        {0, 0, 15} };

   p.take("FACETS") << torus_facets();
   p.take("DIM") << 2;
   p.take("COORDINATES") << Matrix<int>(Coordinates);
   p.take("MANIFOLD") << true;
   p.take("CLOSED_PSEUDO_MANIFOLD") << true;
   p.take("ORIENTED_PSEUDO_MANIFOLD") << true;
   return p;
}

UserFunction4perl("# @category Producing from scratch\n"
                  "# The Császár Torus. Geometric realization by Frank Lutz,\n"
                  "# Electronic Geometry Model No. 2001.02.069\n"
                  "# @return SimplicialComplex",
                  &torus, "torus()");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
