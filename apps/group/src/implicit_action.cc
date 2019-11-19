/* Copyright (c) 1997-2019
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
#include "polymake/Bitset.h"
#include "polymake/group/representations.h"
#include "polymake/group/permlib.h"
#include "polymake/group/group_tools.h"

namespace polymake { namespace group {


namespace {
   // For some reason, this function has to be explicitly specified.
   // Trying to use an instantiated template doesn't work in the line
   // face_orbit.orbit(face, gen_list, pm_set_action);
   // below.
   Bitset pm_set_action(const permlib::Permutation& p, const Bitset& s)
   {
      Bitset simg;
      for (auto i : s)
         simg += p / i;
      return simg;
   }
}


template<typename SetType>
Array<int> implicit_character(perl::Object ia)
{
   const Array<Array<int>> generators = ia.give("STRONG_GENERATORS | GENERATORS");
   const ConjugacyClassReps<Array<int>> ccr = ia.give("CONJUGACY_CLASS_REPRESENTATIVES");
   const Array<SetType> orbit_reps = ia.give("EXPLICIT_ORBIT_REPRESENTATIVES");

   typedef permlib::Permutation PERM;
   std::list<PERM::ptr> permlib_gens;
   for (const auto& perm : generators) {
      PERM::ptr gen(new permlib::Permutation(perm.begin(),perm.end()));
      permlib_gens.push_back(gen);
   }

   std::vector<PERM::ptr> permlib_cc_reps;
   for (const auto& perm : ccr) {
      PERM::ptr g_ptr(new permlib::Permutation(perm.begin(), perm.end()));
      permlib_cc_reps.push_back(g_ptr);
   }

   Array<int> character(ccr.size());
   for (const auto& rep : orbit_reps) {
      permlib::OrbitSet<PERM, SetType> face_orbit;
      face_orbit.orbit(rep, permlib_gens, pm_set_action);
      for (const auto& o_elem : face_orbit) {
         int i(0);
         for (const auto& g_ptr : permlib_cc_reps) {
            const SetType image(pm_set_action(*g_ptr, o_elem));
            if (image == o_elem) {
               ++character[i];
            }
            ++i;
         }
      }
   }
   return character;
}



UserFunction4perl("# @category Symmetry"
		  "# Calculate character of an implicit action"
                  "# @param ImplicitActionOnSets A the given action"
                  "# @return Array<Int>",
                  &implicit_character<Bitset>, "implicit_character(ImplicitActionOnSets)");

}
}


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:

