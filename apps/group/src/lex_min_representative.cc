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
#include "polymake/Map.h"
#include "polymake/linalg.h"
#include "polymake/group/permlib.h"

namespace polymake { namespace group {

template <typename SetType>
SetType lex_min_representative(perl::Object G, const SetType& S)
{
   const PermlibGroup group = group_from_perl_action(G);
   const SetType R = group.lex_min_representative(S);
   return R;
}

template<typename SetType>
std::pair<Array<SetType>, Array<int> >
orbit_reps_and_sizes(const Array<Array<int> >& generators,
                     const Array<SetType>& domain)
{
   const PermlibGroup group(generators);

   Map<SetType, int> orbit_size;
   for (typename Entire<Array<SetType> >::const_iterator dit = entire(domain); !dit.at_end(); ++dit)
      ++orbit_size[group.lex_min_representative(*dit)];

   Array<SetType> reps(orbit_size.size());
   Array<int> size(orbit_size.size());
   typename Entire<Array<SetType> >::iterator rit = entire(reps);
   Entire<Array<int> >::iterator sit = entire(size);

   for (typename Entire<Map<SetType,int> >::const_iterator oit = entire(orbit_size); !oit.at_end(); ++oit, ++rit, ++sit) {
      *rit = oit->first;
      *sit = oit->second;
   }

   return std::make_pair<Array<SetType>, Array<int> >(reps, size);
}


UserFunctionTemplate4perl("# @category Symmetry"
                          "# Computes the lexicographically smallest representative of a given set with respect to a group"
                          "# @param Group G a symmetry group"
                          "# @param Set S a set" 
                          "# @return Set the lex-min representative of S"
                          "# @example To calculate the lex-min representative of the triangle [2,5,7] under the symmetry group of the 3-cube, type"
                          "# > print lex_min_representative(cube_group(3)->PERMUTATION_ACTION, new Set([2,5,7]));"
                          "# | {0 1 6}",
                          "lex_min_representative<SetType>(PermutationAction SetType)");
 
FunctionTemplate4perl("orbit_reps_and_sizes<SetType>(Array<Array<Int>>, Array<SetType>)");

} }


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
