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

#include "polymake/client.h"
#include "polymake/Array.h"
#include "polymake/Graph.h"
#include "polymake/list"

namespace polymake { namespace tropical {

Array<int> ch2d_3phases(const int n, const Array< Array< Set<int> > >& Types, const Graph<>& G)
{
   std::list<int> cyclic;

   // find lower left pseudovertex; its first type entry contains all generators
   int current=0;
   while (Types[current][0].size()<n) ++current;
   cyclic.push_back(current);

   // go through the three phases
   for (int phase=1; phase<=3; ++phase) {
      while (true) {
         const int sector=phase%3, other_sector=(phase-1)%3;
         Entire<Graph<>::out_edge_list>::const_iterator e=entire(G.out_edges(current));
         int x=e.to_node(); ++e;
         while (!e.at_end()) {
            const int primary_compare=incl(Types[x][sector],Types[e.to_node()][sector]);
            if (primary_compare<0 ||
                (primary_compare==0 && incl(Types[x][other_sector],Types[e.to_node()][other_sector])<0))
               x=e.to_node();
            ++e;
         }
         if (incl(Types[current][sector],Types[x][sector])<=0) {
            current=x;
            cyclic.push_back(current);
         } else
            break;
      }
   }

   return Array<int>(cyclic);
}

UserFunction4perl("# @category Other"
                  "# List the pseudovertices of a 2-dimensional tropical polytope on the boundary"
                  "# in counter-clockwise cyclic order."
                  "# @param Int n the number of generators"
                  "# @param Array<Array<Set>> Types the types of the generators"
                  "# @param Graph G the [[PSEUDOVERTEX_GRAPH]]"
                  "# @return Array<int> the pseudovertices on the boundary",
                  &ch2d_3phases, "ch2d_3phases");


} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
