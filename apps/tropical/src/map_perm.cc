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
#include "polymake/Vector.h"
#include "polymake/Matrix.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Map.h"

namespace polymake { namespace tropical {

/*
 * @brief Takes a map (i,j)->... and a permutation on one of the factors of the domain and
 * returns the permutated map
 * @param Map< std::pair<Int, Int>, E> orig The original map
 * @param Permutation A permutation on the first or second factor of the domain {(i,j)}.
 * @param bool first Whether the first (true) or the second factor (false) is being permuted
 * @return The permuted map
 */
template <typename E, typename Permutation>
Map<std::pair<Int, Int>, E> permute_map(const Map<std::pair<Int, Int>, E>& orig, const Permutation& perm, bool first)
{
  Map<std::pair<Int, Int>, E> result;
  for (auto map = entire(orig); !map.at_end(); ++map) {
    Int arg1 = first ? perm[map->first.first] : map->first.first;
    Int arg2 = first ? map->first.second : perm[map->first.second];
    result[std::pair<Int, Int>(arg1,arg2)] = map->second;
  }
  return result;
} //END permute_map

template <typename E, typename Permutation>
Map<std::pair<Int, Int>, E> permute_map_first_factor(const Map<std::pair<Int, Int>, E>& orig, const Permutation& perm)
{
  return permute_map(orig,perm,true);
}

template <typename E, typename Permutation>
Map<std::pair<Int, Int>, E> permute_map_second_factor(const Map<std::pair<Int, Int>, E>& orig, const Permutation& perm)
{
  return permute_map(orig,perm,false);
}

FunctionTemplate4perl("permute_map_first_factor<E,P>(Map<Pair<Int,Int>,E>,P)");
	
FunctionTemplate4perl("permute_map_second_factor<E,P>(Map<Pair<Int,Int>,E>,P)");

} }
