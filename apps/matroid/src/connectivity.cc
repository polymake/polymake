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
#include "polymake/Array.h"
#include "polymake/PowerSet.h"
#include "polymake/list"
#include "polymake/EquivalenceRelation.h"

namespace polymake { namespace matroid {
namespace {

Array<Set<int>> connected_components_from_circuits(const Set<Set<int>>& circuits, const int n) //cast Array to Set, sorting the sets
{
   EquivalenceRelation components(n);
   for(auto c : circuits) {
      components.merge_classes(c);
   }
   return Array<Set<int> >(components.equivalence_classes());
}

}//end anonymous namespace

Function4perl(&connected_components_from_circuits, "connected_components_from_circuits");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
