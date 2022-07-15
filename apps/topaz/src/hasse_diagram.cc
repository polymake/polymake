/* Copyright (c) 1997-2022
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
#include "polymake/topaz/hasse_diagram.h"

namespace polymake { namespace topaz {

using namespace graph;
using namespace graph::lattice;

BigObject hasse_diagram_caller(BigObject complex, const RankRestriction& rr)
{
   const Array<Set<Int>>& facets = complex.give("FACETS");
   return static_cast<BigObject>(hasse_diagram_from_facets(facets, rr));
}

BigObject hasse_diagram(BigObject complex)
{
   return hasse_diagram_caller(complex, RankRestriction());
}

BigObject upper_hasse_diagram(BigObject complex, Int boundary_rank)
{
   return hasse_diagram_caller(complex, RankRestriction(true, RankCutType::GreaterEqual, boundary_rank));
}

Function4perl(&hasse_diagram, "hasse_diagram(SimplicialComplex)");
Function4perl(&upper_hasse_diagram, "upper_hasse_diagram(SimplicialComplex, $)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
