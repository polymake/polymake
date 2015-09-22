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
#include "polymake/Array.h"
#include "polymake/group/permlib.h"
#include "polymake/common/boost_dynamic_bitset.h"
#include "polymake/group/quotiented_representation.h"

namespace polymake { namespace group {

typedef common::boost_dynamic_bitset SetType;

namespace {
   // For some reason, this function has to be explicitly specified. 
   // Trying to use an instantiated template doesn't work in the line
   // face_orbit.orbit(face, gen_list, pm_set_action);
   // below.
   SetType pm_set_action(const permlib::Permutation& p, const SetType& s) 
   {
      SetType simg;
      simg.resize(s.capacity());
      BOOST_FOREACH(unsigned long i, s) 
         simg += p / i;
      return simg;
   }
}

Array<int> quotiented_character(perl::Object R)
{
   const Array<Array<int> > generators = R.give("GROUP.GENERATORS");
   const Array<Array<int> > ccr = R.give("GROUP.CONJUGACY_CLASS_REPRESENTATIVES");
   const Array<SetType> domain = R.give("DOMAIN");

   typedef permlib::Permutation PERM;
   std::list<PERM::ptr> gen_list;
   for(Entire<Array<Array<int> > >::const_iterator perm = entire(generators); !perm.at_end(); ++perm){
      PERM::ptr gen(new permlib::Permutation((*perm).begin(),(*perm).end()));
      gen_list.push_back(gen);
   }
   
   Array<int> character(ccr.size());
   for (Entire<Array<SetType> >::const_iterator dit = entire(domain); !dit.at_end(); ++dit) {
      permlib::OrbitSet<PERM, SetType> face_orbit;
      face_orbit.orbit(*dit, gen_list, pm_set_action);
      for (permlib::OrbitSet<PERM, SetType>::const_iterator oit = face_orbit.begin(), oend=face_orbit.end(); oit!=oend; ++oit) {
         for (int i=0; i<ccr.size(); ++i) {
            PERM::ptr rep(new permlib::Permutation(ccr[i].begin(), ccr[i].end()));
            const SetType image(pm_set_action(*rep, *oit));
            if (image == *oit)
               ++character[i];
         }
      }
   }
   return character;
}

UserFunction4perl("# @category Other"
		  "# Calculate character of quotiented representation"
                  "# @param QuotientedPermutationRepresentation the given representation"
                  "# @return Array<Int>",
                  &quotiented_character, "quotiented_character(QuotientedPermutationRepresentation)");


}
}


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:

