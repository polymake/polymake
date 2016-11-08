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
#include "polymake/group/sparse_isotypic_components.h"
#include "polymake/group/group_tools.h"
#include "polymake/group/permlib.h"
#include "polymake/common/boost_dynamic_bitset.h"

namespace polymake { namespace group {

namespace {

template <typename SparseSet>
auto
sparse_isotypic_spanning_set_and_support_cached(int order,
                                                const Array<Array<int>>& original_generators,
                                                const ConjugacyClasses& conjugacy_classes,
                                                const Vector<Rational>& character,
                                                const Array<SparseSet>& induced_orbit_representatives,
                                                const std::string& filename = "",
                                                bool calculate_support = true,
                                                bool equivalence_class_only = true,
                                                bool index_only = true)
{
   std::vector<SparseSimplexVector<SparseSet>> spanning_hash_vectors;
   hash_set<SparseSet> support;
   std::vector<int> support_indices;

   const Rational c0_ord(character[0] / order);
   std::ofstream outfile;
   if (filename != "" && filename != "-")
      outfile = std::ofstream(filename.c_str(), std::ios_base::trunc);
   std::ostream& os = (filename == "-" ? perl::cout : outfile);

   // for measured efficiency reasons, we once and for all allocate a SparseSet to receive permutations
   SparseSet working_set;
   working_set.resize(original_generators[0].size());
   Rational working_a(0);

   // cache information about the group
   const GroupIndex index_of(group_index(conjugacy_classes));
   const int n(index_of.size());
   std::vector<int> class_of(n);
   std::vector<const Array<int>*> g_of_i; g_of_i.reserve(n);
   for (int j=0; j<conjugacy_classes.size(); ++j) {
      for (const auto& c : conjugacy_classes[j]) {
         class_of[index_of.at(c)] = j;
         g_of_i.push_back(&c);
      }
   }
   const GroupMultiplicationTable GMT(group_multiplication_table_impl(conjugacy_classes, index_of));
   std::vector<int> inverse_of(n);
   for (int i=0; i<n; ++i)
      for (int j=0; j<n; ++j)
         if (GMT[i][j]==0) {
            inverse_of[i] = j;
            break;
         }
   const PermlibGroup original_group(original_generators);

   int orep_index(0);
   for (const auto& orep : induced_orbit_representatives) {
      /*
        For each orbit representative orep, we must execute the loop
        for  f = h orep  in orbit(orep):
           for  c_i  in conjugacy_classes:
              for  g  in c_i:
                 equation[g h orep] += chi_i * chi_0/|Gamma|

        We do this by first calculating and indexing the stabilizer of orep:
       */
      const auto entire_stab(all_group_elements_impl(original_group.setwise_stabilizer(orep)));
      std::vector<int> indexed_stab; indexed_stab.reserve(entire_stab.size());
      for (const auto& p : entire_stab)
         indexed_stab.push_back(index_of.at(p));

      /*
        Next, we partition Gamma into translates of the stabilizer H, such that Gamma is the disjoint union
        Gamma = bigcup_{r in pr} r*H
       */
      const Array<int> pr(partition_representatives_impl(indexed_stab, GMT));
      bool found_support(false);

      SparseSimplexVector<SparseSet> old_hash_eq;
      for (const auto& h1 : pr) {
         SparseSimplexVector<SparseSet> new_hash_eq;
         /*
           We use f1 = h1 orep.
           For each h2 in pr, we use f2 = h2 orep and calculate
             new_hash_eq[f2] = (chi_0/|Gamma|) sum_{g in Gamma : g f1 = f2} chi(class(g))
           where
             { g in Gamma : g f1 = f2 } = h2 Stab(orep) h1^{-1}
           We use class( h2 s h1^-1 ) = class(h1^-1 h2 s)
         */
         for (const auto& h2 : pr) {
            const Array<int>& GMT_h1inv_h2(GMT[GMT[inverse_of[h1]][h2]]);
            working_a = 0;
            for (const auto& s : indexed_stab) {
               working_a += character[class_of[GMT_h1inv_h2[s]]];
            }
            working_a *= c0_ord;
            group::permute_to(entire(orep), *g_of_i[h2], working_set);
            new_hash_eq[working_set] = working_a; // by construction of pr, working_set has a different value in each iteration, so no "+="
         }
         if (new_hash_eq == old_hash_eq) continue; // guard against the most trivial repetition
         if (calculate_support) {
            for (const auto m : new_hash_eq) {
               if (!is_zero(m.second)) {
                  found_support = true;
                  if (equivalence_class_only) {
                     break; // escape enumeration of orbit
                  }
                  support += m.first;
               }
            }
         } else {
            if (filename.size())
               wrap(os) << new_hash_eq << endl;
            else
               spanning_hash_vectors.push_back(new_hash_eq);
         }
         old_hash_eq = new_hash_eq;
      }
      if (calculate_support && equivalence_class_only && found_support) {
         if (index_only)
            support_indices.push_back(orep_index);
         else
            support += orep;
         if (filename.size()) {
            if (index_only) wrap(os) << orep_index << " ";
            else wrap(os) << orep;
            os.flush();
         }
      }
      ++orep_index;
   }
   if (calculate_support && !equivalence_class_only && filename.size())
      wrap(os) << support << endl;
   
   return std::make_pair(spanning_hash_vectors, support);
}

} // end anonymous namespace

auto
sparse_isotypic_basis(const perl::Object& group,
                      const perl::Object& implicit_action,
                      int irrep,
                      perl::OptionSet options)
{
   const int order = group.give("ORDER");
   const Array<Array<int>> generators = implicit_action.give("STRONG_GENERATORS | GENERATORS");
   const ConjugacyClasses conjugacy_classes = implicit_action.give("CONJUGACY_CLASSES");
   const Matrix<Rational> character_table = group.give("CHARACTER_TABLE");
   const Array<common::boost_dynamic_bitset> orbit_representatives = implicit_action.give("EXPLICIT_ORBIT_REPRESENTATIVES");
   const bool use_double = options["use_double"];
   const std::string filename = options["filename"];

   return use_double
      ? sparse_isotypic_basis_impl<common::boost_dynamic_bitset, double>(order, generators, conjugacy_classes, character_table[irrep], orbit_representatives, filename)
      : sparse_isotypic_basis_impl(order, generators, conjugacy_classes, character_table[irrep], orbit_representatives, filename);
}

Array<SparseSimplexVector<common::boost_dynamic_bitset>>
sparse_isotypic_spanning_set(const perl::Object& group,
                             const perl::Object& implicit_action,
                             int irrep,
                             perl::OptionSet options)
{
   const int order = group.give("ORDER");
   const Array<Array<int>> generators = implicit_action.give("STRONG_GENERATORS | GENERATORS");
   const ConjugacyClasses conjugacy_classes = implicit_action.give("CONJUGACY_CLASSES");
   const Matrix<Rational> character_table = group.give("CHARACTER_TABLE");
   const Array<common::boost_dynamic_bitset> orbit_representatives = implicit_action.give("EXPLICIT_ORBIT_REPRESENTATIVES");
   const std::string filename = options["filename"];
   const bool cached = options["cached"];

   return cached
      ? Array<SparseSimplexVector<common::boost_dynamic_bitset>>(sparse_isotypic_spanning_set_and_support_cached(order, generators, conjugacy_classes, character_table[irrep], orbit_representatives, filename, false).first)
      : Array<SparseSimplexVector<common::boost_dynamic_bitset>>(sparse_isotypic_spanning_set_and_support_impl(order, generators, conjugacy_classes, character_table[irrep], orbit_representatives, filename, false).first);
}

auto
sparse_isotypic_support(const perl::Object& group,
                        const perl::Object& implicit_action,
                        int irrep,
                        perl::OptionSet options)
{
   const int order = group.give("ORDER");
   const Array<Array<int>> generators = implicit_action.give("STRONG_GENERATORS | GENERATORS");
   const ConjugacyClasses conjugacy_classes = implicit_action.give("CONJUGACY_CLASSES");
   const Matrix<Rational> character_table = group.give("CHARACTER_TABLE");
   const Array<common::boost_dynamic_bitset> orbit_representatives = implicit_action.give("EXPLICIT_ORBIT_REPRESENTATIVES");
   const std::string filename = options["filename"];
   const bool cached = options["cached"];
   const bool equivalence_class_only = options["equivalence_class_only"];
   const bool index_only = options["index_only"];

   return cached 
      ? sparse_isotypic_spanning_set_and_support_cached(order, generators, conjugacy_classes, character_table[irrep], orbit_representatives, filename, true, equivalence_class_only, index_only).second
      : sparse_isotypic_spanning_set_and_support_impl(order, generators, conjugacy_classes, character_table[irrep], orbit_representatives, filename, true).second;
}

bool spans_invariant_subspace(perl::Object implict_action,
                              const SparseIsotypicBasis<common::boost_dynamic_bitset>& subspace_generators,
                              perl::OptionSet options)
{
   const bool verbose = options["verbose"];
   const Array<Array<int>> group_generators = implict_action.give("STRONG_GENERATORS | GENERATORS");
   return spans_invariant_subspace_impl(group_generators, subspace_generators, verbose);
}

template<typename SetType>
bool span_same_subspace(const SparseIsotypicBasis<SetType>& b1, const SparseIsotypicBasis<SetType>& b2)
{
   hash_map<SetType, int> index_of;
   augment_index_of(index_of, b1);
   augment_index_of(index_of, b2);
   return null_space(list_matrix_representation(index_of, b1)) == null_space(list_matrix_representation(index_of, b2));
}

UserFunction4perl("# @category Symmetry"
                  "# Calculate a sparse representation of a basis for an isotypic component."
                  "# For this, the sets in the representation are listed in order by orbit. In this basis,"
                  "# the projection matrix to the isotypic component decomposes into blocks, one for each orbit."
                  "# @param PermutationActionOnSets the given representation"
                  "# @param Int the index of the irrep that defines the isotypic component"
                  "# @option Bool use_double use inexact arithmetic for reducing the basis; default 0"
                  "# @option String filename if defined, the basis will be written to a file with this name, but not returned."
                  "# Use this option if you expect very large output."
                  "# @return Array<HashMap<boost_dynamic_bitset,Rational>> Basis. Each entry tells the coefficient for each orbit representative.",
                  &sparse_isotypic_basis,
                  "sparse_isotypic_basis(group::Group group::ImplicitActionOnSets $ { use_double => 0, filename => undef })");

UserFunction4perl("# @category Symmetry"
                  "# Calculate a sparse representation of a spanning set for an isotypic component."
                  "# For this, the sets in the representation are listed in order by orbit. In this basis,"
                  "# the projection matrix to the isotypic component decomposes into blocks, one for each orbit."
                  "# @param PermutationActionOnSets the given representation"
                  "# @param Int the index of the irrep that defines the isotypic component"
                  "# @option String filename if defined, the basis will be written to a file with this name, but not returned."
                  "# Use this option if you expect very large output."
                  "# @options Bool cached true for using precomputed group multiplication tables, default false"
                  "# @return Array<HashMap<boost_dynamic_bitset,Rational>> SpanningSet. Each entry tells the coefficient for each orbit representative.",
                  &sparse_isotypic_spanning_set,
                  "sparse_isotypic_spanning_set(group::Group group::ImplicitActionOnSets $ { filename => undef, cached => 0 })");

UserFunction4perl("# @category Symmetry"
                  "# Calculate the support of a sparse representation of a spanning set for an isotypic component."
                  "# @param PermutationActionOnSets the given representation"
                  "# @param Int the index of the irrep that defines the isotypic component"
                  "# @option String filename if defined, the basis will be written to a file with this name, but not returned."
                  "# Use this option if you expect very large output."
                  "# @options Bool cached true for using precomputed group multiplication tables, default false"
                  "# @options Bool equivalence_class_only only report representatives of simplices, default true"
                  "# @options Bool index_only only output the indices of the representatives to filename, default true"
                  "# @return HashSet<boost_dynamic_bitset> Support.",
                  &sparse_isotypic_support,
                  "sparse_isotypic_support(group::Group group::ImplicitActionOnSets $ { filename => undef, cached => 0, equivalence_class_only => 1, index_only => 1 })");

UserFunction4perl("# @category Symmetry"
                  "# Does a set //S// of sparse vectors span an invariant subspace under an implicit group action //a//?"
                  "# @param group::ImplicitActionOnSets a the given action"
                  "# @param Array<HashMap<common::boost_dynamic_bitset, Rational>> S the sparsely given generating vectors of the subspace"
                  "# @option Bool verbose give a certificate if the answer is False"
                  "# @return Bool",
                  &spans_invariant_subspace,
                  "spans_invariant_subspace(group::ImplicitActionOnSets Array<HashMap<common::boost_dynamic_bitset, Rational>> { verbose => 0 })");

UserFunctionTemplate4perl("# @category Symmetry"
                          "# Do two collections //S1//, //S2// of sparse vectors span the same subspace?"
                          "# @param Array<HashMap<SetType, Rational>> S1 the sparse generating vectors of the first subspace"
                          "# @param Array<HashMap<SetType, Rational>> S2 the sparse generating vectors of the second subspace"
                          "# @return Bool",
                          "span_same_subspace<SetType>(Array<HashMap<SetType, Rational>> Array<HashMap<SetType, Rational>>)");

}
}


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:

