/* Copyright (c) 1997-2023
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

#include "polymake/group/group_tools.h"
#include "polymake/group/permlib.h"

namespace polymake { namespace group {

Array<Int> partition_representatives(const Array<Array<Int>>& group_generators,
                                     const Set<Int>& S)
{
   const PermlibGroup G(group_generators);
   Array<Array<Array<Int>>> fake_cc(1);
   fake_cc[0] = all_group_elements_impl(G);
   const GroupIndex<Array<Int>> index_of(group_index(fake_cc));
   const GroupRightMultiplicationTable GMT(group_right_multiplication_table_impl(fake_cc, index_of));
   const auto stab (all_group_elements_impl(G.setwise_stabilizer(S)));
   std::vector<Int> indexed_subgroup;
   for (const auto& s : stab)
      indexed_subgroup.push_back(index_of.at(s));
   return partition_representatives_impl(indexed_subgroup, GMT);
}


UserFunction4perl("# @category Symmetry"
                  "# Partition a group into translates of a set stabilizer"
                  "# @param Array<Array<Int>> gens the generators of a given group action"
                  "# @param Set<Int> S a set"
                  "# @return Array<Int>",
                  &partition_representatives,
                  "partition_representatives(Array<Array<Int>>, Set<Int>)");


} }


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:

