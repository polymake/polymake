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

#include "polymake/client.h"
#include "polymake/group/permlib.h"

namespace polymake { namespace group { 

template <typename Container>
Set<Container> orbit_permlib (BigObject G, const Container& c)
{
   const Array<Array<Int>> generators = G.give("STRONG_GENERATORS | GENERATORS");
   const PermlibGroup sym_group(generators);
   return sym_group.orbit(c);
}

UserFunction4perl("# @category Orbits\n"
                  "# The orbit of a set //S// under a group defined by //G//."
                  "# @param PermutationAction G"
                  "# @param Set S"
                  "# @return Set"
                  "# @example "
                  "# > $G=new Group(PERMUTATION_ACTION=>(new PermutationAction(GENERATORS=>[[1,2,0]])));"
                  "# > print $G->PERMUTATION_ACTION->ALL_GROUP_ELEMENTS;"
                  "# | 0 1 2"
                  "# | 1 2 0"
                  "# | 2 0 1"
                  "# > $S=new Set<Int>(1,2);"
                  "# > print orbit_permlib($G->PERMUTATION_ACTION, $S);"
                  "# | {{0 1} {0 2} {1 2}}",
                  &orbit_permlib<Set<Int>>, "orbit_permlib(PermutationAction, Set)");

UserFunction4perl("# @category Orbits\n"
                  "# The orbit of a set //S// of sets under a group given by //G//."
                  "# @param PermutationAction G"
                  "# @param Set<Set> S"
                  "# @return Set"
                  "# @example"
                  "# > $G=new PermutationAction(new PermutationAction(GENERATORS=>[[2,0,1]]));"
                  "# > print $G->ALL_GROUP_ELEMENTS;"
                  "# | 0 1 2"
                  "# | 2 0 1"
                  "# | 1 2 0"
                  "# > $S=new Set<Set<Int>>(new Set<Int>(1,2), new Set<Int>(0,2));"
                  "# > print orbit_permlib($G, $S);"
                  "# | {{{0 1} {0 2}} {{0 1} {1 2}} {{0 2} {1 2}}}",
                  &orbit_permlib<Set<Set<Int>>>, "orbit_permlib(PermutationAction, Set<Set>)");

} } // end namespaces

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:


