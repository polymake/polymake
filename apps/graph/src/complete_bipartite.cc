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

namespace polymake { namespace graph {

BigObject complete_bipartite(const Int k, const Int l)
{
   if (k < 1 || l < 1)
      throw std::runtime_error("number of nodes on both parts must be positive");

   Graph<> g(k+l);
   for (Int i = 0; i < k; ++i)
     for (Int j = k; j < k+l; ++j)
       g.edge(i,j);

   BigObject G("Graph<>",
               "N_NODES", k+l,
               "N_EDGES", k*l,
               "DIAMETER", 2,
               "CONNECTED", true,
               "BIPARTITE", true,
               "SIGNATURE", std::abs(l-k),
               "ADJACENCY", g);
   G.set_description()<<"Complete bipartite graph on "<<k<<" + "<<l<< " nodes."<<endl;
   return G;

}
UserFunction4perl("# @category Producing a graph"
                  "# Constructs a __complete bipartite graph__ on //k// + //l// nodes."
                  "# @param Int k"
                  "# @param Int l"
                  "# @return Graph"
                  "# @example To print the adjacency representation of a complete bipartite graph"
                  "# with two nodes per partition, type this:"
                  "# > print complete_bipartite(2,2)->ADJACENCY;"
                  "# | {2 3}"
                  "# | {2 3}"
                  "# | {0 1}"
                  "# | {0 1}",
                  &complete_bipartite, "complete_bipartite");


} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
