/* Copyright (c) 1997-2019
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

perl::Object complete(const int n)
{
   if (n < 1)
      throw std::runtime_error("number of nodes must be positive");

   Graph<> g(n);
   for (int i=0; i<n-1; ++i)
     for (int j=i+1; j<n; ++j)
       g.edge(i,j);
   perl::Object G("Graph<>");
   G.take("N_NODES")   << n;
   G.take("N_EDGES")   << (n*(n-1)/2);
   G.take("DIAMETER")  << 1;
   G.take("CONNECTED") << true;
   G.take("BIPARTITE") << (n<=2);
   G.take("ADJACENCY") << g;
   G.set_description() << "Complete graph on "<<n<< " nodes."<<endl;
   return G;

}
UserFunction4perl("# @category Producing a graph\n"
                  "# Constructs a __complete graph__ on //n// nodes."
                  "# @param Int n"
                  "# @return Graph"
                  "# @example To print the adjacency representation of the complete graph on 3 nodes, type this:"
                  "# > print complete(3)->ADJACENCY"
                  "# | {1 2}"
                  "# | {0 2}"
                  "# | {0 1}",
                  &complete, "complete");


} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
