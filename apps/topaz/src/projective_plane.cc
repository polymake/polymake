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

namespace polymake { namespace topaz {
  
Array< Set<int> > projective_plane_facets()
{
   static const int Proj_plane[10][3]={ { 0, 1, 4 },
					{ 0, 1, 5 },
					{ 0, 2, 3 },
					{ 0, 2, 4 },
					{ 0, 3, 5 },
					{ 1, 2, 3 },
					{ 1, 2, 5 },
					{ 1, 3, 4 },
					{ 2, 4, 5 },
					{ 3, 4, 5 } };
   return Array< Set<int> >(Proj_plane);
}

perl::Object projective_plane()
{
   perl::Object p("SimplicialComplex");
   p.set_description() << "The projective plane.\n";

   p.take("FACETS") << projective_plane_facets();
   p.take("DIM") << 2;
   p.take("MANIFOLD") << 1;
   p.take("CLOSED_PSEUDO_MANIFOLD") << 1;
   p.take("ORIENTED_PSEUDO_MANIFOLD") << 0;
   return p;
}

UserFunction4perl("# @category Producing from scratch\n"
		  "# The projective plane.\n"
		  "# @return SimplicialComplex",
		  &projective_plane, "projective_plane()");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
