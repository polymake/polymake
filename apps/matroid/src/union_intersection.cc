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

#include "polymake/client.h"
#include "polymake/Array.h"
#include "polymake/Vector.h"
#include "polymake/Set.h"

namespace polymake { namespace matroid {

   /*
    * @brief For two lists of subsets of the same ground set, computes the list of pairwise unions that
    * have maximal cardinality.
    */
   Array<Set<int> > basis_union(const Array<Set<int> > &b1, const Array<Set<int> > &b2) {
      int max_size = 0;
      Set<Set<int> > result;
      for(Entire<Array<Set<int> > >::const_iterator i1 = entire(b1); !i1.at_end(); i1++) {
         for(Entire<Array<Set<int> > >::const_iterator i2 = entire(b2); !i2.at_end(); i2++) {
            Set<int> union_set = (*i1) + (*i2);
            int us_size = union_set.size();
            if(us_size >= max_size) {
               if(us_size > max_size) {
                  result.clear();
                  max_size = us_size;
               }
               result += union_set;
            }
         }
      }
      return Array<Set<int> >(result);
   }

   // Computes matroid union
   perl::Object matroid_union(const Array<perl::Object>& matroid_list)
   {
      if (matroid_list.empty())
        throw std::runtime_error("Matroid union: Empty list of matroids");
      Array<Set<int>> result_bases;
      const int n = matroid_list[0].give("N_ELEMENTS");

      for (const perl::Object& m : matroid_list) {
         const Array<Set<int>> m_bases = m.give("BASES");
         if (result_bases.empty())
            result_bases = m_bases;
         else
            result_bases = basis_union(result_bases, m_bases);
      }

      perl::Object result("Matroid");
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

   InsertEmbeddedRule("# @category Producing a matroid from matroids\n"
                      "# Computes the intersection of a list of matroids.\n"
                      "# Intersection is the dual of matroid union v, i.e.\n"
                      "# the intersection of M and N is (M* v N*)*\n"
                      "# @param Matroid M A list of matroids, defined on the same ground set.\n"
                      "# @return Matroid The intersection of all matroids in M\n"
                      "user_function intersection {\n"
                      "    return dual(union(map {$_->DUAL} @_));\n"
                      "}\n");

}}
