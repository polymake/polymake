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
#include "polymake/PowerSet.h"
#include "polymake/topaz/graph.h"
#include "polymake/topaz/complex_tools.h"

namespace polymake { namespace topaz {

Graph<> dual_graph(const FacetList& C)
{
   Graph<> DG(C.size());

   for (auto facet=entire(C);  !facet.at_end();  ++facet)
      for (auto face=entire(all_subsets_less_1(*facet)); !face.at_end();  ++face)
         for (auto neighbor_facet=C.findSupersets(*face);  !neighbor_facet.at_end();  ++neighbor_facet)
            if ( (&*facet != &*neighbor_facet) && (neighbor_facet->size() == facet->size()))
               DG.edge(facet.index(), neighbor_facet.index());

   return DG;
}

FunctionTemplate4perl("vertex_graph(*)");
Function4perl(&dual_graph, "dual_graph");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
