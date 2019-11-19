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
#include "polymake/PowerSet.h"
#include "polymake/Matrix.h"
#include "polymake/Rational.h"
#include "polymake/linalg.h"
#include "polymake/list"
#include "polymake/Integer.h"
#include "polymake/matroid/deletion_contraction.h"

namespace polymake { namespace matroid {

   Map<int,int> relabeling_map(const int total_set_size, const Set<int> &removed_set) {
      Map<int,int> result;
      int next_index = 0;
      for(int i = 0; i < total_set_size; i++) {
         if(!removed_set.contains(i)) {
            result[i] = next_index; next_index++;
         }
      }
    
      return result;
   }

   //FIXME This would be more efficient, if refactored into a big object type Minor (see issue #898)

   template <typename MinorType>
   perl::Object minor( perl::Object matroid, Set<int> minor_set, perl::OptionSet options) {
      Array<std::string> list_computed_properties = options["computed_properties"];
      Set<std::string> computed_properties(list_computed_properties);

      int n = matroid.give("N_ELEMENTS");
      //This re-indexes and has nothing to do with [[LABELS]]
      Map<int,int> label_map = relabeling_map(n, minor_set);

      perl::Object result_matroid("Matroid");
         result_matroid.take("N_ELEMENTS") << (n - minor_set.size());

      Array<std::string> labels;
      if (matroid.lookup("LABELS") >> labels && !labels.empty()) {
         result_matroid.take("LABELS") << select(labels, ~minor_set);
      }

      if (computed_properties.size() == 0  || computed_properties.contains(std::string("BASES"))) {
         const Array<Set<int>> m_bases = matroid.give("BASES");
         const Array<Set<int>> result_bases{ minor_bases(MinorType(), m_bases, minor_set, label_map) };
         result_matroid.take("BASES") << result_bases;
         computed_properties -= std::string("BASES");
      }

      if (computed_properties.contains(std::string("CIRCUITS"))) {
         const Array<Set<int>> m_circuits = matroid.give("CIRCUITS");
         const Array<Set<int>> result_circuits = minor_circuits(MinorType(), m_circuits, minor_set, label_map);
         result_matroid.take("CIRCUITS") << result_circuits;
         computed_properties -= std::string("CIRCUITS");
      }

      if (computed_properties.contains(std::string("VECTORS"))) {
         const Matrix<Rational> m_vectors = matroid.give("VECTORS");
         const Matrix<Rational> result_vectors = minor_vectors(MinorType(), m_vectors, minor_set);
         result_matroid.take("VECTORS") << result_vectors;
         computed_properties -= std::string("VECTORS");
      }

      if (!computed_properties.empty())
         throw std::runtime_error("Computing minor: Invalid properties");
      
      return result_matroid;
   }

   // For convenience and backward compat.
   template <typename MinorType>
      perl::Object single_element_minor( perl::Object matroid, int element, perl::OptionSet options) {
         return minor<MinorType>( matroid, scalar2set(element), options);
      }




UserFunction4perl("# @category Producing a matroid from matroids"
                  "# The matroid obtained from a matroid //m// by __deletion__ of set //S// ."
                  "# @param Matroid m"
                  "# @param Set<Int> S indices of elements to be deleted"
                  "# @option Array<String> computed_properties This is a list of property names. Allowed are"
                  "# BASES, CIRCUITS and VECTORS. If given, only these properties will be computed"
                  "# from their counterparts in //m//. If none is given, the default is BASES"
                  "# @example This takes the uniform matroid of rank 2 on 3 elements and deletes the first"
                  "# two elements. It first only computes CIRCUITS and VECTORS, not BASES."
                  "# The second computation only computes the bases."
                  "# > $u = uniform_matroid(2,3);"\
                  "# > $d = deletion( $u, (new Set([0,1])), computed_properties=>[qw(CIRCUITS VECTORS)]);"
                  "# > print join(\",\",$d->list_properties());"
                  "# | N_ELEMENTS,CIRCUITS,VECTORS"
                  "# > $d2 = deletion($u, new Set([0,1]));"
                  "# > print join(\",\",$d2->list_properties());"
                  "# | N_ELEMENTS,BASES"
                  "# @return Matroid",
                  &minor<Deletion>,"deletion(Matroid,Set<Int>, {computed_properties=>[]})");

UserFunction4perl("# @category Producing a matroid from matroids"
                  "# The matroid obtained from a matroid //m// by __deletion__ of element //x// ."
                  "# @param Matroid m"
                  "# @param Int x index of element to be deleted"
                  "# @option Array<String> computed_properties This is a list of property names. Allowed are"
                  "# BASES, CIRCUITS and VECTORS. If given, only these properties will be computed"
                  "# from their counterparts in //m//. If none is given, the default is BASES"
                  "# @return Matroid",
                  &single_element_minor<Deletion>,"deletion(Matroid,Int, {computed_properties=>[]})");

UserFunction4perl("# @category Producing a matroid from matroids"
                  "# The matroid obtained from a matroid //m// by __contraction__ of set //S// ."
                  "# @param Matroid m"
                  "# @param Set<Int> S indices of elements to be contracted"
                  "# @option Array<String> computed_properties This is a list of property names. Allowed are"
                  "# BASES, CIRCUITS and VECTORS. If given, only these properties will be computed"
                  "# from their counterparts in //m//. If none is given, the default is BASES"
                  "# @example This takes the uniform matroid of rank 2 on 3 elements and contracts the first"
                  "# two elements. It first only computes CIRCUITS and VECTORS, not BASES."
                  "# The second computation only computes the bases."
                  "# > $u = uniform_matroid(2,3);"\
                  "# > $d = contraction( $u, (new Set([0,1])), computed_properties=>[qw(CIRCUITS VECTORS)]);"
                  "# > print join(\",\",$d->list_properties());"
                  "# | N_ELEMENTS,CIRCUITS,VECTORS"
                  "# > $d2 = contraction($u, new Set([0,1]));"
                  "# > print join(\",\",$d2->list_properties());"
                  "# | N_ELEMENTS,BASES"
                  "# @return Matroid",
                  &minor<Contraction>,"contraction(Matroid,$, {computed_properties=>[]})");

UserFunction4perl("# @category Producing a matroid from matroids"
                  "# The matroid obtained from a matroid //m// by __contraction__ of element //x// ."
                  "# @param Matroid m"
                  "# @param Int x index of element to be contracted"
                  "# @option Array<String> computed_properties This is a list of property names. Allowed are"
                  "# BASES, CIRCUITS and VECTORS. If given, only these properties will be computed"
                  "# from their counterparts in //m//. If none is given, the default is BASES"
                  "# @return Matroid",
                  &single_element_minor<Contraction>,"contraction(Matroid,Int, {computed_properties=>[]})");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
