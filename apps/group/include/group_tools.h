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

#ifndef __GROUP_TOOLS_H
#define __GROUP_TOOLS_H

#include "polymake/group/action_datatypes.h"
#include "polymake/hash_set"

namespace polymake { namespace group {

typedef Array<Array<int>> GroupMultiplicationTable;
typedef hash_map<Array<int>, int> GroupIndex;

template<typename ConjugacyClasses>
GroupIndex
group_index(const ConjugacyClasses& cc)
{
   int n(0);
   GroupIndex index_of;
   for (const auto& c : cc)
      for (const auto& g : c)
         index_of[g] = n++;
   return index_of;
}

template<typename ConjugacyClasses>
GroupMultiplicationTable
group_multiplication_table_impl(const ConjugacyClasses& cc,
                                const GroupIndex& index_of)
{
   const int n(index_of.size());
   GroupMultiplicationTable GMT(n);
   for (int i=0; i<n; ++i)
      GMT[i].resize(n);
   
   int i(0);
   for (const auto& c : cc) {
      for (const auto& g : c) {
         const ActionType<Array<int>> a(g);
         int j(-1);
         for (const auto& c1 : cc)
            for (const auto& g1 : c1)
               GMT[++j][i] = index_of.at(a(g1)); // transpose the table because of the definition of the action
         ++i;
      }
   }
   return GMT;
}

template<typename IndexedGroup>
Array<int>
partition_representatives_impl(const IndexedGroup& H,
                               const GroupMultiplicationTable& GMT)
{
   const int n(GMT.size()/H.size());
   Array<int> reps(n);
   hash_set<int> remaining(sequence(0, GMT.size()));
   Entire<Array<int>>::iterator rit = entire(reps);
   while (remaining.size()) {
      const int r = remaining.front();
      *rit = r; ++rit;
      const Array<int>& GMT_r(GMT[r]);
      for (const auto& h : H) {
#if POLYMAKE_DEBUG
         if (!remaining.contains(GMT_r[h])) throw std::runtime_error("partition_representatives: could not find expected element");
#endif
         remaining -= GMT_r[h];
      }
#if POLYMAKE_DEBUG
      if (remaining.size() && rit.at_end()) throw std::runtime_error("partition_representatives: incorrect number of reps");
#endif
   }
   return reps;
}


} }

#endif // __GROUP_TOOLS_H


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:


