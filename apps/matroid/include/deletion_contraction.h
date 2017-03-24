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

#ifndef __POLYMAKE_MATROID_DELETION_CONTRACTION_H__
#define __POLYMAKE_MATROID_DELETION_CONTRACTION_H__

#include "polymake/Rational.h"
#include "polymake/Array.h"
#include "polymake/Set.h"
#include "polymake/Map.h"
#include "polymake/FacetList.h"
#include "polymake/linalg.h"
#include "polymake/list"

namespace polymake { namespace matroid {

   struct Deletion;

   struct Contraction {
      typedef Deletion dual;
      const static bool is_deletion = 0;
      const static bool is_contraction = 1;
   };

   struct Deletion {
      typedef Contraction dual;
      const static bool is_deletion = 1;
      const static bool is_contraction = 0;
   };

   /*
    * @brief Takes a set S = (0,..,n-1) (with its induced ordering) and a subset T of that set.
    * Will then return a map from S \ T to int, which for each remaining element gives it its
    * new index if T is taken from S and the remaining ordered elements of S are shifted "to the left".
    * @param int total_set_size The size of S, i.e. S = {0,..,total_set_size -1}
    * @param Set<int> removed_set A subset of S
    * @return Map<int,int>
    */
   Map<int,int> relabeling_map(const int total_set_size, const Set<int> &removed_set);


   // Computes the bases of either a deletion or contraction of a matroid
   template <typename MinorType, typename TBases, typename TMinorSet>
   Set<Set<int>> minor_bases(MinorType,
                             const TBases& bases, const GenericSet<TMinorSet, int>& minor_set,
                             const Map<int, int>& label_map)
   {
      Set<Set<int>> reduced_bases;

      // Go through all bases, take out the minor set and check if it can be a basis
      for (const auto& base : bases) {
         const Set<int> red_set{ label_map[base - minor_set] };
         const int red_size = red_set.size();
         if (!reduced_bases.empty()) {
            const int current_cardinality = reduced_bases.front().size();
            if (red_size != current_cardinality) {
               if (MinorType::is_deletion ? red_size < current_cardinality : red_size > current_cardinality)
                  continue;
               reduced_bases.clear();
            }
         }
         reduced_bases += red_set;
      }

      return reduced_bases;
   }

   // Computes the circuits of a contraction or deletion
   template <typename TCircuits, typename TMinorSet>
   Array<Set<int>> minor_circuits(Deletion,
                                  const TCircuits& circuits, const GenericSet<TMinorSet, int>& minor_set,
                                  const Map<int,int>& label_map)
   {
      std::list<Set<int>> reduced_circuits;

      for (const auto& circuit : circuits) {
         if ((circuit * minor_set).empty())
            reduced_circuits.push_back(Set<int>{ label_map[circuit] });
      }

      return Array<Set<int> >(reduced_circuits);
   }

   template <typename TCircuits, typename TMinorSet>
   Array<Set<int>> minor_circuits(Contraction,
                                  const TCircuits& circuits, const GenericSet<TMinorSet, int>& minor_set,
                                  const Map<int,int>& label_map)
   {
      FacetList reduced_circuits;
      for (const auto& circuit : circuits) {
         const Set<int> red_set{ label_map[circuit - minor_set] };
         if (!red_set.empty())
            reduced_circuits.insertMin(red_set);
      }
      return Array<Set<int> >(reduced_circuits);
   }


   // Computes the vectors of a deletion or contraction
   template <typename TMatrix, typename E, typename TMinorSet>
   Matrix<E> minor_vectors(Deletion, const GenericMatrix<TMatrix, E>& vectors, const GenericSet<TMinorSet, int>& minor_set)
   {
      return vectors.minor(~minor_set, All);
   }

   template <typename TMatrix, typename E, typename TMinorSet>
   Matrix<E> minor_vectors(Contraction, const GenericMatrix<TMatrix, E>& vectors, const GenericSet<TMinorSet, int>& minor_set)
   {
      const int n = vectors.rows();
      Matrix<E> ns1 = null_space(T(vectors));
      if (ns1.rows() > 0)  {
         Matrix<E> ns2 = null_space(ns1.minor(All, ~minor_set));
         if (ns2.rows() > 0)
            return T(ns2);
         else
            return vector2col(zero_vector<E>(n-minor_set.top().size()));
      }
      else {
         return unit_matrix<E>(n-minor_set.top().size());
      } 
   }
} }

#endif // __POLYMAKE_MATROID_DELETION_CONTRACTION_H__

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
