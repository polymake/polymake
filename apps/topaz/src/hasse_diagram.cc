/* Copyright (c) 1997-2015
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

void hasse_diagram_client(perl::Object p)
{
   const Array< Set<int> > C=p.give("FACETS");
   const bool pure=p.give("PURE");
   const int dim=p.give("DIM");
   const HasseDiagram HD=pure ? pure_hasse_diagram(C) : hasse_diagram(C,dim);
   p.take("HASSE_DIAGRAM") << HD.makeObject();
}

Function4perl(&hasse_diagram_client, "hasse_diagram(SimplicialComplex)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
