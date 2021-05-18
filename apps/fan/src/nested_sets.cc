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

#include "polymake/client.h"
#include "polymake/Array.h"
#include "polymake/Set.h"
#include "polymake/PowerSet.h"
#include <list>

namespace polymake { namespace fan {

PowerSet<Int> building_set(const PowerSet<Int>& B, Int n)
{
   PowerSet<Int> building_set;
   for (Int i = 0; i < n; ++i)
      building_set += scalar2set(i);

   std::list<Set<Int>> queue;
   for (auto ait = entire(B); !ait.at_end(); ++ait)
      queue.push_back(*ait);

   while (queue.size()) {
      const Set<Int> b = queue.front(); queue.pop_front();
      building_set += b;
      for (auto pit = entire(building_set); !pit.at_end(); ++pit) {
         const Set<Int> p(*pit);
         if ((p * b).size() > 0) {
            const Set<Int> u(p + b);
            if (!building_set.contains(u)) {
               queue.push_back(u);
            }
         }
      }
   }

   return building_set;
}

bool is_building_set(const PowerSet<Int>& building_set, Int n)
{
   for (Int i = 0; i < n; ++i)
      if (!building_set.contains(scalar2set(i))) {
         cout << "The set does not contain the singleton " << i << endl;
         return false;
      }
   for (auto p = entire(all_subsets_of_k(building_set,2)); !p.at_end(); ++p) {
      const PowerSet<Int> pair(*p);
      const Set<Int>& a = pair.front();
      const Set<Int>& b = pair.back();
      if ((a*b).empty()) continue;
      if (!building_set.contains(a+b)) {
         cout << "The set does not contain the union of " << a << " and " << b << endl;
         return false;
      }
   }
   return true;
}

bool is_B_nested(const PowerSet<Int>& nested, const PowerSet<Int>& building)
{
   for (auto nit = entire(nested); !nit.at_end(); ++nit) {
      if (!building.contains(*nit)) {
         cout << "The building set does not contain " << *nit << endl;
         return false;
      }
   }
   for (auto p=entire(all_subsets_of_k(nested,2)); !p.at_end(); ++p) {
      const PowerSet<Int> pair(*p);
      const Set<Int>& a = pair.front();
      const Set<Int>& b = pair.back();
      Int incl_rel;
      if (!((a*b).empty() || (incl_rel = incl(a,b)) ==  1 || incl_rel == -1)) {
         cout << "The sets " << a << " and " << b << " are not nested." << endl;
         return false;
      }
   }
   for (Int k = 2; k <= nested.size(); ++k) {
      for (auto p = entire(all_subsets_of_k(nested,k)); !p.at_end(); ++p) {
         const PowerSet<Int> family(*p);
         bool pairwise_intersections_empty(true);
         for (auto f=entire(all_subsets_of_k(family,2)); !f.at_end() && pairwise_intersections_empty; ++f) {
            const PowerSet<Int> pair(*f);
            pairwise_intersections_empty = ((pair.front() * pair.back()).size() == 0);
         }
         if (!pairwise_intersections_empty) continue;
         const Set<Int> the_union = accumulate(family, operations::add());
         if (building.contains(the_union)) {
            cout << "The building set contains the union " << the_union << " of the sets " << family << endl;
            return false;
         }
      }
   }
   return true;
}

UserFunction4perl("# @category Other"
                  "# Produce a building set from a family of sets."
                  "# @param Array<Set> generators the generators of the building set"
                  "# @param Int n the size of the ground set"
                  "# @return Set<Set<Int>> the induced building set",
                  &building_set, "building_set(Array<Set> $)");

UserFunction4perl("# @category Other"
                  "# Check if a family of sets is a building set."
                  "# @param Set<Set<Int>> check_me the would-be building set"
                  "# @param Int n the size of the ground set"
                  "# @return Bool is check_me really a building set?",
                  &is_building_set, "is_building_set(Set<Set<Int>> $)");

UserFunction4perl("# @category Other"
                  "# Check if a family of sets is nested wrt a given building set."
                  "# @param Set<Set<Int>> check_me the would-be nested sets"
                  "# @param Set<Set<Int>> B the building set"
                  "# @return Bool is the family of sets really nested wrt B?",
                  &is_B_nested, "is_B_nested(Set<Set<Int>> Set<Set<Int>>)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
