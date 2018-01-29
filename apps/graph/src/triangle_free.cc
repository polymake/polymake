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
#include "polymake/Graph.h"
#include "polymake/IncidenceMatrix.h"

namespace polymake { namespace graph {

template <typename Graph>
bool triangle_free(const GenericGraph<Graph,Undirected>& G)
{
   const int n=G.nodes();
   const IncidenceMatrix<> Adj=convolute(convolute(adjacency_matrix(G), T(adjacency_matrix(G))),
                                         T(adjacency_matrix(G)));

   // true iff there is no (directed) circle of length 3
   for (int i=0; i<n; ++i)
      if (Adj(i,i)) return false;

   return true;
}

FunctionTemplate4perl("triangle_free(props::Graph<Undirected>)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
