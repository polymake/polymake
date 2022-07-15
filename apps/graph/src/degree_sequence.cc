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
#include "polymake/Graph.h"
#include "polymake/Rational.h"

namespace polymake { namespace graph {

void degree_sequence(BigObject G)
{
   Map<Int, Int> ds;
   Int sum_degree = 0;
   const Graph<> g = G.give("ADJACENCY");
   for (Int i = 0; i < g.nodes(); ++i) {
      const Int deg = g.out_degree(i);
      ds[deg]++;
      sum_degree += deg;
   }
   G.take("DEGREE_SEQUENCE") << ds;
   G.take("AVERAGE_DEGREE") << Rational(sum_degree, g.nodes());
}

Function4perl(&degree_sequence, "degree_sequence(Graph)");

} }


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
