/* Copyright (c) 1997-2016
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

#include "polymake/group/group_tools.h"
#include <algorithm>

namespace polymake { namespace group {

GroupMultiplicationTable
group_multiplication_table(perl::Object g, perl::OptionSet options)
{
   const std::string action = options["action"];
   Array<Array<Array<int>>> group_classes(1);
   if (!(g.lookup(action + ".CONJUGACY_CLASSES") >> group_classes)) {
      const Array<Array<int>> gens = g.give(action + ".GENERATORS");
      group_classes[0] = orbit<on_container>(gens, Array<int>(sequence(0, gens[0].size())));
   }
   std::sort(group_classes[0].begin(), group_classes[0].end(), pm::operations::lt<const Array<int>&, const Array<int>&>());
   return group_multiplication_table_impl(group_classes, group_index(group_classes));
}

UserFunction4perl("# @category Symmetry"
                  "# Calculate the multiplication table of a group action"
                  "# @param Group G"
                  "# @option String action which action to take for the calculation; default PERMUTATION_ACTION"
                  "# @return Array<Array<Int>> T the multiplication table, where the elements of //G// are"
                  "# ordered by conjugacy classes (if available), else in generated order",
                  &group_multiplication_table,
                  "group_multiplication_table(Group { action=>'PERMUTATION_ACTION' })");


} }


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:

