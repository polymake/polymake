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
#include "polymake/Matrix.h"
#include "polymake/Rational.h"
#include "polymake/PowerSet.h"
#include "polymake/Array.h"
#include "polymake/list"

namespace polymake { namespace matroid {

/*
 * calculates the rank of a set in a matroid given by his bases 
 */
Int rank_of_set(const Set<Int>& set, const Set<Set<Int>>& bases)
{
   Int rank = 0;
   for (auto it = entire(bases); !it.at_end(); ++it) {
      Int r = ((*it)*set).size();
      if (r > rank)
         rank = r;
   }
   return rank;
}

ListReturn matroid_plueckervector(BigObject matroid)
{
   const Set<Set<Int>> bases = matroid.give("BASES");
   const Int r = matroid.give("RANK");
   const Int n = matroid.give("N_ELEMENTS");
   const Int k = Int(Integer::binom(n, r));
   Vector<Int> char_vec(k);
   Vector<Int> rank_vec(k);

   Int l = 0;
   for (auto i = entire(all_subsets_of_k(sequence(0, n), r)); !i.at_end(); ++i, ++l) {
      if (bases.contains(*i)) {
         char_vec[l] = 1;
         rank_vec[l] = r;
      } else {
         rank_vec[l] = rank_of_set(*i, bases);
      }
   }

   ListReturn list_ret;
   list_ret << char_vec
	    << rank_vec;
   return list_ret;
}

BigObject matroid_from_characteristic_vector(const Vector<Integer>& vec, const Int r, const Int n)
{
   if (vec.dim() != Integer::binom(n,r)) {
      throw std::runtime_error("matroid_from_characteristic_vector: dimension of the vector does not fit with the given rank and the number of elements");
   }
   std::list<Set<Int>> bases;
   Int n_bases = 0;
   Int j = 0;

   // test for each subset of size r
   for (auto i=entire(all_subsets_of_k(sequence(0,n), r)); !i.at_end(); ++i, ++j) {
      if (vec[j]==1) {
	 bases.push_back(*i);
	 ++n_bases;
      }
   }

   return BigObject("Matroid",
                    "BASES", bases,
                    "N_BASES", n_bases,
                    "RANK", r,
                    "N_ELEMENTS", n);
}

UserFunction4perl("# @category Producing a matroid from other objects\n"
                  "# Creates the matroid with a given characteristic plueckervector of rank //r// and a ground set of //n// elements."
                  "# @param Vector<Integer> v"
                  "# @param Int r"
                  "# @param Int n"
                  "# @return Matroid",
                  &matroid_from_characteristic_vector, "matroid_from_characteristic_vector");

UserFunction4perl("# @category Other\n"
                  "# Creates the characteristic- and the rank-plueckervector of a matroid."
                  "# @param Matroid m"
                  "# @return List (Vector<Integer>, Vector<Integer>)",
                  &matroid_plueckervector, "matroid_plueckervector");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
