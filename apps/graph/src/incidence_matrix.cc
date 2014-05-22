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
#include "polymake/SparseMatrix.h"
#include "polymake/Graph.h"

namespace polymake { namespace graph {

namespace {

SparseMatrix<int> incidence_matrix_impl(perl::Object p, int s=1)
{
   const Graph<> G = p.give("ADJACENCY");
   SparseMatrix<int> E (G.nodes(), G.edges());
   int col(0);
   for (Entire<Edges<Graph<> > >::const_iterator eit = entire(edges(G)); !eit.at_end(); ++eit, ++col) {
      E(eit.from_node(), col) = s;
      E(eit.to_node(), col) = 1;
   }
   return E;
}

} // end anonymous namespace

SparseMatrix<int> incidence_matrix(perl::Object p)
{
   return incidence_matrix_impl(p, 1);
}

SparseMatrix<int> signed_incidence_matrix(perl::Object p)
{
   return incidence_matrix_impl(p, -1);
}


UserFunction4perl("# @category Combinatorics"
		  "# Compute the unsigned vertex-edge incidence matrix of the graph."
		  "# @return SparseMatrix<Int>",
                  &incidence_matrix, "incidence_matrix($)");

UserFunction4perl("# @category Combinatorics"
		  "# Compute the signed vertex-edge incidence matrix of the graph."
		  "# in case of undirected graphs, the orientation of the edges is induced by the order of the nodes."
		  "# @return SparseMatrix<Int>",
                  &signed_incidence_matrix, "signed_incidence_matrix($)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
