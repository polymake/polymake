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
#include "polymake/Rational.h"
#include "polymake/SparseMatrix.h"
#include "polymake/SparseVector.h"
#include "polymake/Graph.h"

namespace polymake { namespace polytope {

/*
 * Creates the stable set polytope for a graph G=(V,E)
 * i.e.: x_i + x_j <= 1  forall {i,j} in E
 *             x_i >= 0  forall i in V
 *             x_i <= 1  forall i in V with deg(i)=0
 */
perl::Object stable_set(const perl::Object& G_in)
{
    const Graph<> G = G_in.give("ADJACENCY");          // getting the Graph
    SparseMatrix<Rational> ineqs(G.edges(), G.nodes()+1);   // inequalities of the new polytope
    int row(0);                                        // keeping track of the rows
 
    // For every edge {i,j} create the inequality x_i + x_j <= 1
    for (auto eit = entire(edges(G)); !eit.at_end(); ++eit, ++row) {
       ineqs(row, 0) = 1;
       ineqs(row, eit.from_node()+1) = -1;
       ineqs(row, eit.to_node()+1) = -1;
    }
   
    // non-negative constraints
    ineqs /= unit_matrix<Rational>(G.nodes()+1);

    // adding constraints x_i <= 1 for nodes with degree 0
    // so it does not happen to be unbounded
    int i = 0;
    for (auto vit = entire(nodes(G)); !vit.at_end(); ++vit, ++i) {
       if (vit.degree() == 0) {
         ineqs /= (1 | -unit_vector<Rational>(G.nodes(), i));
       }
    }

    // output
    perl::Object p_out("Polytope<Rational>");
    p_out.set_description() << "stable set polytope of a Graph" << endl;   
    p_out.take("INEQUALITIES") << ineqs;
    p_out.take("BOUNDED") << true;
    p_out.take("ONE_VERTEX") << unit_vector<Rational>(G.nodes()+1,0);
    p_out.take("FEASIBLE") << true;

    return p_out;
}

UserFunction4perl("# @category Producing a polytope from scratch"
                  "# Produces the stable set polytope from an undirected graph //G//=(V,E)."
                  "# The stable set Polytope has the following inequalities:"
                  "#     x_i + x_j <= 1  forall {i,j} in E"
                  "#           x_i >= 0  forall i in V"
                  "#           x_i <= 1  forall i in V with deg(i)=0"
                  "# @param Graph G"
                  "# @return Polytope",
                  &stable_set, "stable_set");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
