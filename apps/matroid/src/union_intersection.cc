/* Copyright (c) 1997-2020
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
#include "polymake/Vector.h"
#include "polymake/Set.h"

namespace polymake { namespace matroid {

/*
 * @brief For two lists of subsets of the same ground set, computes the list of pairwise unions that
 * have maximal cardinality.
 */
Array<Set<Int>> basis_union(const Array<Set<Int>>& b1, const Array<Set<Int>>& b2)
{
  Int max_size = 0;
  Set<Set<Int>> result;
  for (auto i1 = entire(b1); !i1.at_end(); ++i1) {
    for (auto i2 = entire(b2); !i2.at_end(); ++i2) {
      Set<Int> union_set = (*i1) + (*i2);
      Int us_size = union_set.size();
      if (us_size >= max_size) {
        if (us_size > max_size) {
          result.clear();
          max_size = us_size;
        }
        result += union_set;
      }
    }
  }
  return Array<Set<Int>>(result);
}

// Computes matroid union
BigObject matroid_union(const Array<BigObject>& matroid_list)
{
  if (matroid_list.empty())
    throw std::runtime_error("Matroid union: Empty list of matroids");
  Array<Set<Int>> result_bases;
  const Int n = matroid_list[0].give("N_ELEMENTS");

  for (const BigObject& m : matroid_list) {
    const Array<Set<Int>> m_bases = m.give("BASES");
    if (result_bases.empty())
      result_bases = m_bases;
    else
      result_bases = basis_union(result_bases, m_bases);
  }

  BigObject result("Matroid");
  result.take("N_ELEMENTS") << n;
  result.take("BASES") << result_bases;
  return result;
}

UserFunction4perl("# @category Producing a matroid from matroids"
                  "# Computes the union of a list of matroids, i.e. the matroid"
                  "# whose independent sets are all unions of independent sets"
                  "# of the given matroids"
                  "# @param Matroid M A list of matroids, defined on the same ground set."
                  "# @return Matroid The union of all matroids in M",
                  &matroid_union,"union(Matroid+)");

InsertEmbeddedRule("# @category Producing a matroid from matroids"
                   "# Computes the intersection of a list of matroids."
                   "# Intersection is the dual of matroid union v,"
                   "# that is, the intersection of M and N is (M* v N*)*"
                   "# @param Matroid M A list of matroids, defined on the same ground set."
                   "# @return Matroid The intersection of all matroids in M\n"
                   "user_function intersection {\n"
                   "    return dual(union(map {$_->DUAL} @_));\n"
                   "}\n");

} }
