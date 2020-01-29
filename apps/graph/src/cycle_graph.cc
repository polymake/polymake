/* Copyright (c) 1997-2020
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

namespace polymake { namespace graph {

BigObject cycle_graph(const Int n)
{
   if (n < 3)
      throw std::runtime_error("need at least 3 nodes");

   Graph<> g(n);
   for (Int i = 0; i < n-1; ++i)
      g.edge(i, i+1);
   g.edge(0, n-1);
   BigObject G("Graph<>");
   G.take("N_NODES") << n;
   G.take("N_EDGES") << n;
   G.take("DIAMETER") << (n%2 ? (n-1)/2 : n/2);
   G.take("CONNECTED") << true;
   G.take("BIPARTITE") << (n%2 == 0);
   G.take("ADJACENCY") << g;
   G.set_description() << "Cycle graph on " << n << " nodes." << endl;
   return G;
}

BigObject wheel_graph(const Int n)
{
   if (n < 3)
      throw std::runtime_error("need at least 3 nodes");

   Graph<> g(n+1);
   for (Int i = 0; i < n-1; ++i) {
      g.edge(i, i+1);
      g.edge(i, n);
   }
   g.edge(0, n-1);
   g.edge(n-1, n);
   BigObject G("Graph<>");
   G.take("N_NODES") << n+1;
   G.take("N_EDGES") << 2*n;
   G.take("DIAMETER") << (n==3 ? 1 : 2);
   G.take("CONNECTED") << true;
   G.take("BIPARTITE") << false;
   G.take("ADJACENCY") << g;
   G.set_description() << "Wheel graph with " << n << " spokes." << endl;
   return G;
}

BigObject path_graph(const Int n)
{
   if (n < 2)
      throw std::runtime_error("need at least 2 nodes");

   Graph<> g(n);
   for (Int i = 0; i < n-1; ++i)
      g.edge(i, i+1);
   BigObject G("Graph<>");
   G.take("N_NODES") << n;
   G.take("N_EDGES") << n-1;
   G.take("DIAMETER") << n-1;
   G.take("CONNECTED") << true;
   G.take("BIPARTITE") << (n%2 == 0);
   G.take("ADJACENCY") << g;
   G.set_description() << "Path graph on " << n << " nodes." << endl;
   return G;
}


UserFunction4perl("# @category Producing a graph"
                  "# Constructs a __cycle graph__ on //n// nodes."
                  "# @param Int n"
                  "# @return Graph"
                  "# @example To print the adjacency representation of the cycle graph on four nodes, type this:"
                  "# > $g = cycle_graph(4);"
                  "# > print $g->ADJACENCY;"
                  "# | {1 3}"
                  "# | {0 2}"
                  "# | {1 3}"
                  "# | {0 2}",
                  &cycle_graph, "cycle_graph");

UserFunction4perl("# @category Producing a graph"
                  "# Constructs a __wheel graph__ with //n// spokes."
                  "# @param Int n"
                  "# @return Graph"
                  "# @example To print the adjacency representation of the wheel graph with five spokes, type this:"
                  "# > $g = wheel_graph(5);"
                  "# > print $g->ADJACENCY;"
                  "# | {1 4 5}"
                  "# | {0 2 5}"
                  "# | {1 3 5}"
                  "# | {2 4 5}"
                  "# | {0 3 5}"
                  "# | {0 1 2 3 4}",
                  &wheel_graph, "wheel_graph");

UserFunction4perl("# @category Producing a graph"
                  "# Constructs a __path graph__ on //n// nodes."
                  "# @param Int n"
                  "# @return Graph",
                  &path_graph, "path_graph");


} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
