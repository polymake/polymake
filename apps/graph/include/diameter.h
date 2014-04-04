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

#ifndef POLYMAKE_GRAPH_DIAMETER_H
#define POLYMAKE_GRAPH_DIAMETER_H

#include "polymake/GenericGraph.h"
#include "polymake/graph/BFSiterator.h"

namespace polymake { namespace graph {

/// Determine the diameter of a graph.
template <typename Graph>
int diameter(const GenericGraph<Graph>& G)
{
   BFSiterator<Graph, Visitor< NodeVisitor<int> > > it(G,0);
   int diam=0;
   for (typename Entire< Nodes<Graph> >::const_iterator n=entire(nodes(G)); !n.at_end(); ++n) {
      for (it.reset(*n); it.unvisited_nodes()>0; ++it) ;
      assign_max(diam, it.node_visitor()[it.last_unvisited()]);
   }
   return diam;
}

} }

#endif // POLYMAKE_GRAPH_DIAMETER_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
