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
#include "polymake/Graph.h"
#include "polymake/Matrix.h"
#include <cmath>

namespace polymake { namespace graph {

template <typename Dir, typename MatrixTop>
EdgeMap<Dir,double> edge_lengths(const Graph<Dir>& G, const GenericMatrix<MatrixTop>& coords)
{
   EdgeMap<Dir,double> L(G);
   Matrix<double> C(coords);
   for (auto e_it=entire(edges(G)); !e_it.at_end(); ++e_it)
      L[*e_it]=std::sqrt(sqr(C[e_it.from_node()]-C[e_it.to_node()]));

   return L;
}

UserFunctionTemplate4perl("# @category Other"
			  "# Compute the lengths of all edges of a given graph //G// from"
			  "# the coordinates //coords// of its nodes."
			  "# @param GraphAdjacency<Directed> G the input graph"
			  "# @param Matrix coords the coordinates of the nodes"
			  "# @return EdgeMap"
			  "# @example [application polytope] The following prints the edge length of the complete graph with 3 nodes"
			  "# and edge lengths given by the coordiantes of the standard 2-simplex:"
           "# > print edge_lengths(complete(3)->ADJACENCY,simplex(2)->VERTICES);"
           "# | 1 1 1.414213562",
			  "edge_lengths(GraphAdjacency Matrix)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
