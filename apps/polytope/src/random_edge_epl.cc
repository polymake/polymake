/* Copyright (c) 1997-2018
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
#include "polymake/list"
#include "polymake/vector"
#include "polymake/Rational.h"
#include "polymake/Graph.h"
#include "polymake/Vector.h"

namespace polymake { namespace polytope {

Vector<Rational> random_edge_epl(const Graph<Directed>& G)
{
   const int nodes=G.nodes();

   std::vector<int> outgoing_edges(nodes);
   Vector<Rational> average_path_length(nodes);
   std::list<int> node_queue;

   for (int n = 0; n < nodes; ++n)          // initialize all nodes
      if (!(outgoing_edges[n] = G.out_degree(n)))
         node_queue.push_back(n);

   while (!node_queue.empty()) {
      const int n = node_queue.front();
      node_queue.pop_front();
      Rational APL(0);
      for (auto e = entire(G.out_edges(n));  !e.at_end();  ++e)
         APL += average_path_length[e.to_node()];

      if (G.out_degree(n)) average_path_length[n] = APL/G.out_degree(n) + 1;

      for (auto e = entire(G.in_edges(n));  !e.at_end();  ++e) {
         const int f = e.from_node();
         if (!(--outgoing_edges[f])) node_queue.push_back(f);
      }
   }

   return average_path_length;
}

UserFunction4perl("# @category Optimization"
                  "# Computes a vector containing the expected path length to the maximum"
                  "# for each vertex of a directed graph //G//."
                  "# The random edge pivot rule is applied."
                  "# @param Graph<Directed> G a directed graph"
                  "# @return Vector<Rational>",
                  &random_edge_epl, "random_edge_epl");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
