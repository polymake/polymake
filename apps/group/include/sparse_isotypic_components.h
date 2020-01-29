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

#ifndef __GROUP_SPARSE_ISOTYPIC_COMPONENTS_H
#define __GROUP_SPARSE_ISOTYPIC_COMPONENTS_H

#include "polymake/group/group_tools.h"
#include "polymake/group/orbit.h"
#include "polymake/hash_set"
#include "polymake/linalg.h"
#include <fstream>

namespace polymake { namespace group {

/*
  return a basis of the isotypic component corresponding to the given character.
  Only rational characters are considered here, for performance reasons.
  The basis is returned as an Array<hash_map<SparseSet, Rational>>.
  The ordering of the rows in the basis is as they appear while processing the orbits of induced_orbit_representatives in lex order.
 */
template <typename SparseSet, typename NumericalType=Rational>
SparseIsotypicBasis<SparseSet>
sparse_isotypic_basis_impl(Int order,
                           const Array<Array<Int>>& original_generators,
                           const ConjugacyClasses<>& conjugacy_classes,
                           const Vector<Rational>& character,
                           const Array<SparseSet>& induced_orbit_representatives,
                           const std::string& filename = "")
{
   const Rational c0_ord(character[0] / order);
   std::ofstream outfile;
   if (filename != "" && filename != "-")
      outfile = std::ofstream(filename.c_str(), std::ios_base::trunc);
   std::ostream& os = (filename == "-" ? perl::cout : outfile);

   // for measured efficiency reasons, we once and for all allocate a SparseSet to receive permutations
   SparseSet working_set(induced_orbit_representatives[0]);
   working_set.clear();
   
   /*
     for each representative of an orbit of facets, we calculate the corresponding block     
         B = sum_{g in Gamma} chi_i(g) phi(g)
     of the projection matrix.
   */

   std::vector<SparseSimplexVector<SparseSet>> basis_hash_vectors;

   for (const auto& orep: induced_orbit_representatives) {
      // The rows and columns of B are indexed by the orbit of orep.
      // One could make the orbit unordered and save the time spent ordering it, but then one would lose knowledge of what simplices rows and columns correspond to.
      const auto face_orbit(orbit<on_container, Array<Int>, SparseSet>(original_generators, orep));
      
      // index the orbit, and create constant-time access to indexed elements
      hash_map<SparseSet, Int> index_of;
      std::vector<SparseSet> face_orbit_indexed;
      face_orbit_indexed.reserve(face_orbit.size());
      Int index = -1;
      for (const auto& f: face_orbit) {
         index_of[f] = ++index;
         face_orbit_indexed.push_back(f);
      }

      // make an explicit ListMatrix to keep track of the linear span achieved so far. 
      // Whenever a new row is added to the ListMatrix, append a corresponding SparseSimplexVector to basis_vectors.
      ListMatrix<SparseVector<NumericalType>>
         class_sparse_eqs(0, face_orbit.size()),
         kernel_so_far(unit_matrix<NumericalType>(face_orbit.size()));

      // for each potential new row of the block B, check if it is linearly independent from what is already there
      for (const auto& f: face_orbit) {
         /* 
            Each phi(g) is a permutation matrix.
            For example, for the permutation (b,a,c) we get the matrix
              a  b  c
                 1     a
              1        b
                    1  c
            Therefore, the row corresponding to f gets a contribution chi_i(g) in the column g(f), for all g in Gamma.
         */
         SparseVector<NumericalType> new_sparse_eq(face_orbit.size());
         for (Int i = 0; i<conjugacy_classes.size(); ++i) {
            if (is_zero(character[i])) 
               continue;
            for (const auto& g: conjugacy_classes[i]) {
               group::permute_to(f.begin(), g, working_set);
               new_sparse_eq[index_of[working_set]] += convert_to<NumericalType>(character[i]);
            }
         }
         
         if (add_row_if_rowspace_increases(class_sparse_eqs, new_sparse_eq, kernel_so_far)) {
            SparseSimplexVector<SparseSet> new_hash_eq;
            for (auto eit = entire(new_sparse_eq); !eit.at_end(); ++eit) {
               // multiply by chi_i(id)/|Gamma|
               new_hash_eq[face_orbit_indexed[eit.index()]] = Rational(*eit) * c0_ord;
            }
            if (filename.size())
               wrap(os) << new_hash_eq << endl;
            else
               basis_hash_vectors.push_back(new_hash_eq);
         }
      }
   }
   
   return SparseIsotypicBasis<SparseSet>(basis_hash_vectors);
}

template <typename SparseSet>
auto
sparse_isotypic_spanning_set_and_support_impl(Int order,
                                              const Array<Array<Int>>& original_generators,
                                              const ConjugacyClasses<>& conjugacy_classes,
                                              const Vector<Rational>& character,
                                              const Array<SparseSet>& induced_orbit_representatives,
                                              const std::string& filename = "",
                                              bool calculate_support = true)
{
   std::vector<SparseSimplexVector<SparseSet>> spanning_hash_vectors;
   hash_set<SparseSet> support;

   const Rational c0_ord(character[0] / order);
   std::ofstream outfile;
   if (filename != "" && filename != "-")
      outfile = std::ofstream(filename.c_str(), std::ios_base::trunc);
   std::ostream& os = (filename == "-" ? perl::cout : outfile);

   SparseSet working_set(induced_orbit_representatives[0]);
   working_set.clear();

   SparseSimplexVector<SparseSet> old_hash_eq;
   for (const auto& orep: induced_orbit_representatives) {
      for (const auto& f: orbit<on_container, Array<Int>, SparseSet>(original_generators, orep)) {
         SparseSimplexVector<SparseSet> new_hash_eq;
         for (Int i = 0; i < conjugacy_classes.size(); ++i) {
            if (is_zero(character[i])) 
               continue;
            for (const auto& g: conjugacy_classes[i]) {
               group::permute_to(f.begin(), g, working_set);
               new_hash_eq[working_set] += character[i] * c0_ord;
            }
         }
         if (new_hash_eq == old_hash_eq) continue; // guard against the most trivial repetition
         old_hash_eq = new_hash_eq;
         if (calculate_support) {
            for (const auto m: old_hash_eq)
               if (!is_zero(m.second))
                  support += m.first;            
         } else  {
            if (filename.size())
               wrap(os) << new_hash_eq << endl;
            else
               spanning_hash_vectors.push_back(new_hash_eq);
         }
      }
   }
   
   if (calculate_support && filename.size())
      wrap(os) << support << endl;
   
   return std::make_pair(Array<SparseSimplexVector<SparseSet>>(spanning_hash_vectors.size(), entire(spanning_hash_vectors)), support);
}

template<typename SparseSet>
void
augment_index_of(hash_map<SparseSet, Int>& index_of,
                 const SparseIsotypicBasis<SparseSet>& subspace_generators)
{
   Int index = index_of.size();
   for (const auto& sgen: subspace_generators)
      for (const auto m: sgen)
         if (!index_of.exists(m.first))
            index_of[m.first] = index++;
}

template<typename SparseSet>
ListMatrix<SparseVector<Rational>>
list_matrix_representation(const hash_map<SparseSet, Int>& index_of,
                           const SparseIsotypicBasis<SparseSet>& subspace_generators)
{
  ListMatrix<SparseVector<Rational>> sgen_matrix(0, index_of.size());
   for (const auto& sgen: subspace_generators) {
      SparseVector<Rational> new_sgen(index_of.size());
      for (const auto m: sgen)
         new_sgen[index_of.at(m.first)] = m.second;
      sgen_matrix /= new_sgen;
   }
   return sgen_matrix;
}

template<typename SparseSet>
bool
spans_invariant_subspace_impl(const Array<Array<Int>>& group_generators,
                              const SparseIsotypicBasis<SparseSet>& subspace_generators,
                              bool verbose)
{
   hash_map<SparseSet, Int> index_of;
   augment_index_of(index_of, subspace_generators);
   const SparseMatrix<Rational> ker = null_space(list_matrix_representation(index_of, subspace_generators));

   for (const auto& sgen: subspace_generators) {
      for (const auto& o_sgen: unordered_orbit<on_container>(group_generators, sgen)) {
         SparseVector<Rational> new_sgen(index_of.size());
         for (const auto m: o_sgen) {
            try {
               new_sgen[index_of.at(m.first)] = m.second;
            } catch (const no_match&) {
               if (verbose) cerr << "The given vectors do not span an invariant subspace, because "
                                 << m << " is in the support of the orbit of " << sgen
                                 << ", but not in the orbit of the support of the given vectors" << endl;
               return false;
            }
         }
         if (!is_zero(ker * new_sgen)) {
            if (verbose) cerr << "The given vectors do not span an invariant subspace, because "
                              << new_sgen << ", corresponding to " 
                              << o_sgen << " is not in the spanned subspace L. Here, ker L =\n"
                              << ker << endl;
            return false;
         }
      }
   }
   return true;
}

} }

#endif // __GROUP_SPARSE_ISOTYPIC_COMPONENTS_H


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:

