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
#include "polymake/Matrix.h"
#include "polymake/Graph.h"
#include "polymake/linalg.h"
#include "polymake/Rational.h"
#include "polymake/graph/bipartite.h"

namespace polymake { namespace topaz {

Int signature(BigObject p)
{
   const Graph<> DG=p.give("DUAL_GRAPH.ADJACENCY");
   Matrix<Rational> GR=p.give("COORDINATES");
   GR = ones_vector<Rational>(GR.rows()) | GR;

   const Array<Set<Int>> F=p.give("FACETS");
   graph::BFSiterator<Graph<>, graph::VisitorTag<graph::BipartiteColoring>> it(DG, nodes(DG).front());
   Int sign = 0;

   try {
      while (!it.at_end()) ++it;

      // if it has survived so far, then the dual graph is really bipartite
      for (Int f = 0, end = F.size(); f < end; ++f)
         if (abs(numerator(det(GR.minor(F[f],All))))%2 == 1)
            sign += it.node_visitor().get_color(f);

      sign = abs(sign);
   } catch (Int) {
      sign = -1;
   }

   return sign;
}

Function4perl(&signature, "signature(SimplicialComplex)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
