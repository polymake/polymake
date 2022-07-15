/* Copyright (c) 1997-2022
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

#include "polymake/topaz/hasse_diagram.h"

namespace polymake { namespace topaz {

// return values: 1=true, 0=false, -1=undef (does not occur here)
template <typename Complex, typename VertexSet>
Int is_ball_or_sphere(const Complex& C, const GenericSet<VertexSet>& V, int_constant<2>)
{
   graph::Lattice<graph::lattice::BasicDecoration> HD = hasse_diagram_from_facets(Array<Set<Int>>(C));

   // check whether C is a pseudo_manifold and compute the boundary B
   std::list<Set<Int>> B;
   bool is_PM = is_pseudo_manifold(HD, true, std::back_inserter(B));
   if (!is_PM) return 0;

   // check whether B is a sphere or empty
   const bool B_is_empty = B.empty();
   if (!B_is_empty && is_ball_or_sphere(B, int_constant<1>())==0) return 0;

   // S:= C + B*v for a vertex v not contained in C
   // note: S is a sphere <=> C is ball or sphere
   // check euler char of S
   // if (B.empty())  S = -1 + #vert - #edges_of_C + #C
   // else            S = -1 + (#vert + 1) - (#edges_of_C + #B) + (#C + #B)
   Int euler_char = V.top().size() - HD.nodes_of_rank(HD.rank()-2).size()+C.size();
   if (B_is_empty) --euler_char;

   if (euler_char!=1) return 0;

   return 1;
}

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
