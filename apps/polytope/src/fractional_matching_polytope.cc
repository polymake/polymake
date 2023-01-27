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
#include "polymake/SparseMatrix.h"
#include "polymake/Vector.h"
#include "polymake/Graph.h"
#include "polymake/Rational.h"
#include "polymake/node_edge_incidences.h"

namespace polymake { namespace polytope {

typedef Graph<Undirected> graph_type;

BigObject fractional_matching_polytope(const graph_type& G)
{
   const SparseMatrix<Rational> M((unit_matrix<Rational>(G.edges()+1)) /
                                  (ones_vector<Rational>() | -node_edge_incidences<Rational>(G)));
   return BigObject("Polytope<Rational>",
                    "INEQUALITIES", M,
                    "BOUNDED", true,
                    "FEASIBLE", true,
                    "ONE_VERTEX", unit_vector<Rational>(G.edges()+1, 0));
}
      
UserFunction4perl("#@category Producing a polytope from graphs"
                  "# Matching polytope of an undirected graph."
                  "# @param Graph G"
                  "# @return Polytope",
                  &fractional_matching_polytope, "fractional_matching_polytope(GraphAdjacency)");
      
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
