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
  
bool is_closed_pseudo_manifold(const HasseDiagram& HD, bool known_pure)
{
   if (HD.in_degree(HD.top_node())==0)
      return true;

   if (!known_pure && !is_pure(HD))
      return false;

   for (Entire<HasseDiagram::nodes_of_dim_set>::const_iterator it=entire(HD.nodes_of_dim(-2)); !it.at_end(); ++it)
      if ( HD.out_degree(*it) != 2 )     // ridge contained in one facet only
         return false;

   return true;
}

void is_closed_pseudo_manifold_client(perl::Object p)
{
   HasseDiagram HD=p.give("HASSE_DIAGRAM");
   p.take("CLOSED_PSEUDO_MANIFOLD") << is_closed_pseudo_manifold(HD,true);
}

Function4perl(&is_closed_pseudo_manifold_client, "is_closed_pseudo_manifold(SimplicialComplex)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
