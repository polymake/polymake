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

#ifndef POLYMAKE_TOPAZ_GRAPH_H
#define POLYMAKE_TOPAZ_GRAPH_H

#include "polymake/Graph.h"
#include "polymake/FacetList.h"

namespace polymake { namespace topaz {

// Computes the graph of a simplicial complex.
template <typename Complex> inline
Graph<> vertex_graph(const Complex& C);

// Computes the dual graph of a simplicial complex.
Graph<> dual_graph(const FacetList& C);

} }

#include "polymake/topaz/graph.tcc"

#endif // POLYMAKE_TOPAZ_GRAPH_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
