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

#ifndef POLYMAKE_GRAPH_BIPARTITE_H
#define POLYMAKE_GRAPH_BIPARTITE_H

#include "polymake/GenericGraph.h"
#include "polymake/vector"
#include "polymake/list"

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

} }

#include "polymake/graph/bipartite.tcc"

#endif // POLYMAKE_GRAPH_BIPARTITE_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
