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
#include "polymake/Rational.h"
#include "polymake/SparseMatrix.h"
#include "polymake/SparseVector.h"
#include "polymake/Graph.h"
#include "polymake/Array.h"
#include "polymake/linalg.h"

namespace polymake { namespace polytope {

/*
 * creating the flow polytope of a directed Graph G=(V,E)
 * with a given source and sink. The flow polytope has the following
 * outer description
 *   forall v in V-{source, sink}:
 *     sum_{e in E going into v} x_e
 *      -  sum_{e in E going out of v} x_e = 0
 *
 *   sum_{e in E going into source} x_e
 *    -  sum_{e in E going out of source} x_e <= 0
 * 
 *   sum_{e in E going into sink} x_e
 *    -  sum_{e in E going out of sink} x_e >= 0
 * 
 *   forall e in E:
 *     x_e <= given bound on edge e
 */
template <typename Scalar>
BigObject flow_polytope(const Graph<Directed> G, const EdgeMap<Directed,Scalar> arc_bounds, Int source, Int sink)
{
   SparseMatrix<Scalar> ineqs(G.edges()+3, G.edges()+1);   // inequalities of the new polytope   
   SparseMatrix<Scalar> eqs(G.nodes()-2, G.edges()+1);     // equations of the new polytope

   Int i = 0; // Keeping track of the number of the current edge.
   for (auto e=entire(edges(G));  !e.at_end();  ++e, ++i) {

      // Inequality: x_e <= given bound on edge e 
      ineqs(i, i+1) = -1;
      ineqs(i, 0) = arc_bounds[i];


      /* 
       * The flow-consistency quations are made without iterating through all the nodes.
       * So whenever we gain information about a node while iterating through the edges
       * we use that to create a part of the equation.
       */


      // distinguishing weather from node is source or sink.
      // creating accordingly either a part of the flow-consistency equation at a node or
      // an inequality at the source or sink
      if (e.from_node() == source) 
         ineqs(G.edges(), i+1) = 1;
      else if (e.from_node() == sink) 
         ineqs(G.edges()+1, i+1) = -1;
      else {
         // make sure to stay in the matrix range since source and sink
         // do not have any equations
         Int offset = (sign(source - e.from_node()) + sign(sink - e.from_node()))/2-1;
         eqs(e.from_node()+offset, i+1) = -1;
      }
      
      // distinguishing weather from node is source or sink.
      // creating accordingly either a part of the flow-consistency equation at a node or
      // an inequality at the source or sink
      if (e.to_node() == source) {
         ineqs(G.edges(), i+1) = -1;
      } else if (e.to_node() == sink) {
         ineqs(G.edges()+1, i+1) = 1;
      } else {
         // make sure to stay in the matrix range since source and sink
         // do not have any equations
         Int offset = (sign(source - e.to_node()) + sign(sink - e.to_node()))/2-1;
         eqs(e.to_node()+offset, i+1) = 1;         
      }
   }
   ineqs(G.edges()+2,0) = 1; // far face

   BigObject p_out("Polytope", mlist<Scalar>(),
                   "INEQUALITIES", ineqs,
                   "EQUATIONS", remove_zero_rows(eqs),
                   "FEASIBLE", true,
                   "VALID_POINT", unit_vector<Scalar>(G.edges()+1, 0));
   p_out.set_description() << "flow polytope of a Graph" << endl;   
   return p_out;
}

template <typename Scalar>
BigObject flow_polytope(const BigObject G_in, const Array<Scalar>& arc_bounds, Int source, Int sink)
{
   Graph<Directed> G = G_in.give("ADJACENCY");
   EdgeMap<Directed,Scalar> EM(G);
   Int i = 0;
   for (auto e = entire(edges(G));  !e.at_end();  ++e, ++i) {
      EM[i] = arc_bounds[i];
   }
   
   return flow_polytope<Scalar>(G, EM, source, sink);
}
UserFunctionTemplate4perl("# @category Producing a polytope from graphs"
                  "# Produces the flow polytope of a directed Graph //G//=(V,E)"
                  "# with a given //source// and //sink//. The flow polytope has the following"
                  "# outer description:"
                  "#   forall v in V-{source, sink}:"
                  "#     sum_{e in E going into v} x_e"
                  "#      -  sum_{e in E going out of v} x_e = 0"
                  "# "
                  "#   sum_{e in E going into source} x_e"
                  "#    -  sum_{e in E going out of source} x_e <= 0"
                  "# "
                  "#   sum_{e in E going into sink} x_e"
                  "#    -  sum_{e in E going out of sink} x_e >= 0"
                  "# "
                  "#   forall e in E:"
                  "#     x_e <= given bound on edge e "
                  "# @param GraphAdjacency<Directed> G"
                  "# @param EdgeMap<Directed, Scalar> Arc_Bounds"
                  "# @param Int source"
                  "# @param Int sink"
                  "# @tparam Scalar"
                  "# @return Polytope",
                  "flow_polytope<Scalar>(GraphAdjacency EdgeMap<Directed,Scalar> $ $)");

UserFunctionTemplate4perl("# @category Producing a polytope from graphs"
                  "# Produces the flow polytope of a directed Graph //G//=(V,E)"
                  "# with a given //source// and //sink//. The flow polytope has the following"
                  "# outer description:"
                  "#   forall v in V-{source, sink}:"
                  "#     sum_{e in E going into v} x_e"
                  "#      -  sum_{e in E going out of v} x_e = 0"
                  "# "
                  "#   sum_{e in E going into source} x_e"
                  "#    -  sum_{e in E going out of source} x_e <= 0"
                  "# "
                  "#   sum_{e in E going into sink} x_e"
                  "#    -  sum_{e in E going out of sink} x_e >= 0"
                  "# "
                  "#   forall e in E:"
                  "#     x_e <= given bound on edge e "
                  "# @param Graph<Directed> G"
                  "# @param Array<Scalar> Arc_Bounds"
                  "# @param Int source"
                  "# @param Int sink"
                  "# @tparam Scalar"
                  "# @return Polytope",
                  "flow_polytope<Scalar>(Graph Array<Scalar> $ $)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
