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

#pragma once

#include "polymake/Graph.h"
#include "polymake/FacetList.h"
#include "polymake/topaz/complex_tools.h"

namespace polymake { namespace topaz {

// Computes the graph of a simplicial complex.
template <typename Complex>
Graph<> vertex_graph(const Complex& C)
{
   const PowerSet<Int> OSK = k_skeleton(C, 1);
   const Set<Int> V = accumulate(OSK, operations::add());

   Graph<> G(V.back()+1);
   for (auto c_it = entire(OSK); !c_it.at_end(); ++c_it)
      if (c_it->size() == 2)  // is edge
         G.edge(c_it->front(), c_it->back());

   return G;
}

// Computes the dual graph of a simplicial complex.
Graph<> dual_graph(const FacetList& C);

} }


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
