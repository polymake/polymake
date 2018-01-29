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

#ifndef POLYMAKE_GRAPH_BIPARTITE_H
#define POLYMAKE_GRAPH_BIPARTITE_H

#include "polymake/GenericGraph.h"
#include "polymake/vector"
#include "polymake/list"
#include "polymake/Set.h"
#include "polymake/Vector.h"

namespace polymake { namespace graph {

/* Determine whether an undirected graph is bipartite.
 * Returns a negative int if not bipartite. If the graph is bipartite,
 * the absolute difference of the black and white colored nodes is returned.
 * Also works for disconnected graphs (albeit its use may be limited).
 *
 * @author Niko Witte
 */

template <typename Graph>
int bipartite_sign(const GenericGraph<Graph,Undirected>& G);


// given a bipartite graph, color it with two colors, 0 and 1
template <typename Graph>
Vector<int> bipartite_coloring(const GenericGraph<Graph,Undirected>& G)
{
   assert(G.nodes() > 0);

   Vector<int> color_of(G.nodes(), 2); // initialize to dummy color 2
   std::list<int> queue;
   queue.push_back(0);
   color_of[0] = 1;
   Set<int> new_nodes(sequence(0, G.nodes()));
   while (queue.size()) {
      const int n = queue.front(); queue.pop_front();
      new_nodes -= n;
      const Set<int> neighbors = G.top().adjacent_nodes(n) * new_nodes;
      const bool color = color_of[n];
      for (Entire<Set<int> >::const_iterator sit = entire(neighbors); !sit.at_end(); ++sit) {
         queue.push_back(*sit);
         if (color_of[*sit] != 2 && color_of[*sit] != !color)
            throw std::runtime_error("Graph is not bipartite");
         color_of[*sit] = !color;
      }
   }
   return color_of;
}

} }

#include "polymake/graph/bipartite.tcc"

#endif // POLYMAKE_GRAPH_BIPARTITE_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
