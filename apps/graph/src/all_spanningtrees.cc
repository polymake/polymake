/* Copyright (c) 1997-2020
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
#include "polymake/Array.h"
#include "polymake/graph/all_spanningtrees.h"


namespace polymake { namespace graph {

Array<Set<Int>> calc_all_spanningtrees(const Graph<>& G)
{
   return all_spanningtrees(G);
}

UserFunction4perl("# @category Combinatorics"
                  "# Calculate all spanning trees for a connected graph along the lines of"
                  "#\t Donald E. Knuth: The Art of Computer Programming, Volume 4, Fascicle 4, 24-31, 2006, Pearson Education Inc."
                  "# @param Graph G beeing connected"
                  "# @return Array<Set<Int>>"
                  "# @example The following prints all spanning trees of the complete graph with"
                  "# 3 nodes, whereby each line represents a single spanning tree as an edge set:"
                  "# > print all_spanningtrees(complete(3)->ADJACENCY);"
                  "# | {0 1}"
                  "# | {1 2}"
                  "# | {0 2}",
                  &calc_all_spanningtrees, "all_spanningtrees");

} }

// Local Variables:
// c-basic-offset:3
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
