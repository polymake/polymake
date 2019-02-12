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
#include "polymake/Set.h"
#include "polymake/PowerSet.h"
#include <list>

namespace polymake { namespace fan {

PowerSet<int> building_set(const Set<Set<int> >& B, int n)
{
   PowerSet<int> building_set;
   for (int i=0; i<n; ++i)
      building_set += scalar2set(i);

   std::list<Set<int> > queue;
   for (auto ait = entire(B); !ait.at_end(); ++ait)
      queue.push_back(*ait);

   while (queue.size()) {
      const Set<int> b = queue.front(); queue.pop_front();
      building_set += b;
      for (auto pit = entire(building_set); !pit.at_end(); ++pit) {
         const Set<int> p(*pit);
         if ((p * b).size() > 0) {
            const Set<int> u (p + b);
            if (!building_set.contains(u)) {
               queue.push_back(u);
            }
         }
      }
   }

   return building_set;
}

bool is_building_set(const PowerSet<int>& building_set, int n)
{
   for (int i=0; i<n; ++i)
      if (!building_set.contains(scalar2set(i))) {
         cout << "The set does not contain the singleton " << i << endl;
         return false;
      }
   for (auto p=entire(all_subsets_of_k(building_set,2)); !p.at_end(); ++p) {
      const Set<Set<int> > pair(*p);
      const Set<int> a(pair.front()), b(pair.back());
      if ((a*b).size() == 0) continue;
      if (!building_set.contains(a+b)) {
         cout << "The set does not contain the union of " << a << " and " << b << endl;
         return false;
      }
   }
   return true;
}

bool is_B_nested(const Set<Set<int> >& nested, const PowerSet<int>& building)
{
   for (auto nit = entire(nested); !nit.at_end(); ++nit) {
      if (!building.contains(*nit)) {
         cout << "The building set does not contain " << *nit << endl;
         return false;
      }
   }
   for (auto p=entire(all_subsets_of_k(nested,2)); !p.at_end(); ++p) {
      const Set<Set<int> > pair(*p);
      const Set<int> a(pair.front()), b(pair.back());
      if (! ( (a*b).size() == 0 ||
              incl(a,b) ==  1 ||
              incl(a,b) == -1) ) {
         cout << "The sets " << a << " and " << b << " are not nested." << endl;
         return false;
      }
   }
   for (int k=2; k <= nested.size(); ++k) {
      for (auto p=entire(all_subsets_of_k(nested,k)); !p.at_end(); ++p) {
         const Set<Set<int> > family(*p);
         bool pairwise_intersections_empty(true);
         for (auto f=entire(all_subsets_of_k(family,2)); !f.at_end() && pairwise_intersections_empty; ++f) {
            const Set<Set<int> > pair(*f);
            pairwise_intersections_empty = ((pair.front() * pair.back()).size() == 0);
         }
         if (!pairwise_intersections_empty) continue;
         const Set<int> the_union = accumulate(family, operations::add());
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
                  "# @return PowerSet the induced building set",
                  &building_set, "building_set(Array<Set> $)");

UserFunction4perl("# @category Other"
                  "# Check if a family of sets is a building set."
                  "# @param PowerSet check_me the would-be building set"
                  "# @param Int n the size of the ground set"
                  "# @return Bool is check_me really a building set?",
                  &is_building_set, "is_building_set(PowerSet $)");

UserFunction4perl("# @category Other"
                  "# Check if a family of sets is nested wrt a given building set."
                  "# @param Set<Set> check_me the would-be nested sets"
                  "# @param PowerSet B the building set"
                  "# @return Bool is the family of sets really nested wrt B?",
                  &is_B_nested, "is_B_nested(Set<Set> PowerSet)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
