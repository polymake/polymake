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
#include "polymake/group/permlib.h"

namespace polymake { namespace group { 

template <typename Container>
Set<Container> orbit (perl::Object G, const Container& c)
{
   const Array<Array<int> > generators = G.give("GENERATORS");
   const PermlibGroup sym_group(generators);
   return sym_group.orbit(c);
}

UserFunction4perl("# @category Orbits\n"
                  "# The orbit of a set //S// under a group //G//."
                  "# @param Group G"
                  "# @param Set S"
                  "# @return Set",
                  &orbit<Set<int> >, "orbit(Group, Set)");

UserFunction4perl("# @category Orbits\n"
                  "# The orbit of a set //S// of sets under a group //G//."
                  "# @param Group G"
                  "# @param Set<Set> S"
                  "# @return Set",
                  &orbit<Set<Set<int> > >, "orbit(Group, Set<Set>)");

} } // end namespaces

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:


