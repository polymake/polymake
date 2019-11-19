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
#include "polymake/group/sparse_isotypic_components.h"
#include "polymake/group/group_tools.h"
#include "polymake/group/permlib.h"
#include "polymake/Bitset.h"

namespace polymake { namespace group {

auto
sparse_isotypic_basis(const perl::Object& group,
                      const perl::Object& implicit_action,
                      int irrep,
                      perl::OptionSet options)
{
   const int order = group.give("ORDER");
   const Array<Array<int>> generators = implicit_action.give("STRONG_GENERATORS | GENERATORS");
   const ConjugacyClasses<> conjugacy_classes = implicit_action.give("CONJUGACY_CLASSES");
   const Matrix<Rational> character_table = group.give("CHARACTER_TABLE");
   const Array<Bitset> orbit_representatives = implicit_action.give("EXPLICIT_ORBIT_REPRESENTATIVES");
   const bool use_double = options["use_double"];
   const std::string filename = options["filename"];

   return use_double
      ? sparse_isotypic_basis_impl<Bitset, double>(order, generators, conjugacy_classes, character_table[irrep], orbit_representatives, filename)
      : sparse_isotypic_basis_impl(order, generators, conjugacy_classes, character_table[irrep], orbit_representatives, filename);
}

Array<SparseSimplexVector<Bitset>>
sparse_isotypic_spanning_set(const perl::Object& group,
                             const perl::Object& implicit_action,
                             int irrep,
                             perl::OptionSet options)
{
   const int order = group.give("ORDER");
   const Array<Array<int>> generators = implicit_action.give("STRONG_GENERATORS | GENERATORS");
   const ConjugacyClasses<> conjugacy_classes = implicit_action.give("CONJUGACY_CLASSES");
   const Matrix<Rational> character_table = group.give("CHARACTER_TABLE");
   const Array<Bitset> orbit_representatives = implicit_action.give("EXPLICIT_ORBIT_REPRESENTATIVES");
   const std::string filename = options["filename"];

   return sparse_isotypic_spanning_set_and_support_impl(order, generators, conjugacy_classes, character_table[irrep], orbit_representatives, filename, false).first;
}

auto
sparse_isotypic_support(const perl::Object& group,
                        const perl::Object& implicit_action,
                        int irrep,
                        perl::OptionSet options)
{
   const int order = group.give("ORDER");
   const Array<Array<int>> generators = implicit_action.give("STRONG_GENERATORS | GENERATORS");
   const ConjugacyClasses<> conjugacy_classes = implicit_action.give("CONJUGACY_CLASSES");
   const Matrix<Rational> character_table = group.give("CHARACTER_TABLE");
   const Array<Bitset> orbit_representatives = implicit_action.give("EXPLICIT_ORBIT_REPRESENTATIVES");
   const std::string filename = options["filename"];
   
   return sparse_isotypic_spanning_set_and_support_impl(order, generators, conjugacy_classes, character_table[irrep], orbit_representatives, filename, true).second;
}                        

bool spans_invariant_subspace(perl::Object implict_action,
                              const SparseIsotypicBasis<Bitset>& subspace_generators,
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
   int rank1 = rank(list_matrix_representation(index_of, b1));
   int rank2 = rank(list_matrix_representation(index_of, b2));
   return rank1 == rank2 && rank1 == rank(list_matrix_representation(index_of, b1)/list_matrix_representation(index_of, b2));
}

UserFunction4perl("# @category Symmetry"
                  "# Calculate a sparse representation of a basis for an isotypic component."
                  "# For this, the sets in the representation are listed in order by orbit. In this basis,"
                  "# the projection matrix to the isotypic component decomposes into blocks, one for each orbit."
                  "# @param PermutationActionOnSets rep the given representation"
                  "# @param Int i the index of the irrep that defines the isotypic component"
                  "# @option Bool use_double use inexact arithmetic for reducing the basis; default 0"
                  "# @option String filename if defined, the basis will be written to a file with this name, but not returned."
                  "# Use this option if you expect very large output."
                  "# @return Array<HashMap<Bitset,Rational>> Basis. Each entry tells the coefficient for each orbit representative.",
                  &sparse_isotypic_basis,
                  "sparse_isotypic_basis(Group ImplicitActionOnSets $ { use_double => 0, filename => undef })");

UserFunction4perl("# @category Symmetry"
                  "# Calculate a sparse representation of a spanning set for an isotypic component."
                  "# For this, the sets in the representation are listed in order by orbit. In this basis,"
                  "# the projection matrix to the isotypic component decomposes into blocks, one for each orbit."
                  "# @param PermutationActionOnSets rep the given representation"
                  "# @param Int i the index of the irrep that defines the isotypic component"
                  "# @option String filename if defined, the basis will be written to a file with this name, but not returned."
                  "# Use this option if you expect very large output."
                   "# @return Array<HashMap<Bitset,Rational>> SpanningSet. Each entry tells the coefficient for each orbit representative.",
                  &sparse_isotypic_spanning_set,
                  "sparse_isotypic_spanning_set(Group ImplicitActionOnSets $ { filename => undef })");

UserFunction4perl("# @category Symmetry"
                  "# Calculate the support of a sparse representation of a spanning set for an isotypic component."
                  "# @param PermutationActionOnSets rep the given representation"
                  "# @param Int i the index of the irrep that defines the isotypic component"
                  "# @option String filename if defined, the basis will be written to a file with this name, but not returned."
                  "# Use this option if you expect very large output."
                  "# @options Bool equivalence_class_only only report representatives of simplices, default true"
                  "# @options Bool index_only only output the indices of the representatives to filename, default true"
                  "# @return HashSet<Bitset> Support.",
                  &sparse_isotypic_support,
                  "sparse_isotypic_support(Group ImplicitActionOnSets $ { filename => undef, cached => 0, equivalence_class_only => 1, index_only => 1 })");

UserFunction4perl("# @category Symmetry"
                  "# Does a set //S// of sparse vectors span an invariant subspace under an implicit group action //a//?"
                  "# @param ImplicitActionOnSets a the given action"
                  "# @param Array<HashMap<Bitset, Rational>> S the sparsely given generating vectors of the subspace"
                  "# @option Bool verbose give a certificate if the answer is False"
                  "# @return Bool",
                  &spans_invariant_subspace,
                  "spans_invariant_subspace(ImplicitActionOnSets Array<HashMap<Bitset, Rational>> { verbose => 0 })");

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

