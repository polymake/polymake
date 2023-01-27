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

#include "polymake/client.h"
#include "polymake/Array.h"
#include "polymake/Set.h"
#include "polymake/Map.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/linalg.h"
#include "polymake/group/permlib.h"

namespace polymake { namespace group {

namespace {

template <typename Iterator>
std::pair<Array<Set<Int>>, Array<Int>>
orbit_reps_and_sizes_impl(const Array<Array<Int>>& generators,
                          Iterator& domain_it)
{
   const PermlibGroup group(generators);

   Map<Set<Int>, Int> orbit_size;
   while (!domain_it.at_end()) {
      ++orbit_size[group.lex_min_representative(Set<Int>(*domain_it))];
      ++domain_it;
   }

   Array<Set<Int>> reps(orbit_size.size());
   Array<Int> size(orbit_size.size());
   auto rit = entire(reps);
   auto sit = entire(size);

   for (auto oit = entire(orbit_size); !oit.at_end(); ++oit, ++rit, ++sit) {
      *rit = oit->first;
      *sit = oit->second;
   }

   return std::make_pair(reps, size);
}

} // end anonymous namespace

template <typename SetType>
SetType lex_min_representative(BigObject G, const SetType& S)
{
   return group_from_perl_action(G).lex_min_representative(S);
}

template <typename Container>
std::pair<Array<Set<Int>>, Array<Int>>
orbit_reps_and_sizes(const Array<Array<Int>>& generators,
                     const Container& domain)
{
   auto row_it = entire(domain);
   return orbit_reps_and_sizes_impl(generators, row_it);
}

template <>
std::pair<Array<Set<Int>>, Array<Int>>
orbit_reps_and_sizes(const Array<Array<Int>>& generators,
                     const IncidenceMatrix<>& domain)
{
   auto row_it = entire(rows(domain));
   return orbit_reps_and_sizes_impl(generators, row_it);
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
 
UserFunctionTemplate4perl("# @category Symmetry"
                          "# Computes the lexicographically smallest representatives of a given array of sets with respect to a group,"
                          "# along with the corresponding orbit sizes"
                          "# @tparam Container a container of sets, for example Array<Set> or IncidenceMatrix"
                          "# @param Array<Array<Int>> generators the generators of a symmetry group"
                          "# @param Container S a container of sets, for example Array<Set> or IncidenceMatrix" 
                          "# @return Pair<Array<Set<Int>>,Array<Int>> the lex-min representatives of S, and the sizes of the corresponding orbits"
                          "# @example To calculate the orbits and orbit sizes of an icosidodecahedron, type"
                          "# > $q=polytope::icosidodecahedron();"
                          "# > print orbit_reps_and_sizes($q->GROUP->VERTICES_ACTION->GENERATORS,$q->VERTICES_IN_FACETS);"
                          "# | <{0 1 2 4 6}"
                          "# | {0 1 3}"
                          "# | >"
                          "# | 12 20",
                          "orbit_reps_and_sizes<Container>(Array<Array<Int>>, Container)");

} }


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
