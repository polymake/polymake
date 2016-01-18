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

#include "polymake/client.h"
#include "polymake/group/representations.h"

namespace polymake { namespace group {

Array<int> irreducible_decomposition(const Array<int>& character, perl::Object G)
{
   const Matrix<Rational> character_table = G.give("CHARACTER_TABLE");
   const Array<int> cc_sizes = G.give("CONJUGACY_CLASS_SIZES");
   const int order = G.give("ORDER");

   Vector<Rational> weighted_character(character.size(), entire(character));
   for (int i=0; i<weighted_character.size(); ++i)
      weighted_character[i] *= cc_sizes[i];

   const Vector<Rational> irr_dec(character_table * weighted_character / order);
   
   Array<int> irr_dec_i (irr_dec.size());
   for (int i=0; i<irr_dec.size(); ++i) {
      if (denominator(irr_dec[i]) != 1) {
         throw std::runtime_error("The given array is not a character of the group.");
      }
      irr_dec_i[i] = convert_to<int>(irr_dec[i]);
   }
   return irr_dec_i;
}

namespace {

inline
bool orbit_ordering(const std::string &ordering) {
   if (ordering == "orbit") return true;
   else if (ordering == "lex") return false;
   else throw std::runtime_error("Unsupported domain ordering");
}

} // end anonymous namespace

SparseMatrix<Rational> rep(perl::Object R, const Array<int>& perm) {
   const int degree = R.give("DEGREE");
   const Array<Set<int> > domain = R.give("DOMAIN");
   const Map<Set<int>,int> index_of = R.give("INDEX_OF");
   return InducedAction<Set<int> >(degree, domain, index_of).rep(perm);
}

SparseMatrix<Rational> isotypic_projector(perl::Object R, int irred_index, perl::OptionSet options) {
   const int degree = R.give("DEGREE");
   const std::string ordering = options["domain_ordering"];
   const Array<Set<int> > domain = R.give(orbit_ordering(ordering) 
                                          ? "DOMAIN_IN_ORBIT_ORDER"
                                          : "DOMAIN");
   const Map<Set<int>,int> index_of = R.give(orbit_ordering(ordering)
                                             ? "INDEX_IN_ORBIT_ORDER_OF"
                                             : "INDEX_OF");
   const int order = R.give("GROUP.ORDER");
   const Matrix<Rational> character_table = R.give("GROUP.CHARACTER_TABLE");
   const Array<Set<Array<int> > > conjugacy_classes = R.give("GROUP.CONJUGACY_CLASSES");

   if (irred_index<0 || irred_index >= character_table.rows())
      throw std::runtime_error("The given index does not refer to an irreducible representation.");

   return isotypic_projector_impl(character_table[irred_index], InducedAction<Set<int> >(degree, domain, index_of), degree, conjugacy_classes, order);
}

SparseMatrix<Rational> isotypic_basis(perl::Object R, int irred_index, perl::OptionSet options) {
   const int degree = R.give("DEGREE");
   const std::string ordering = options["domain_ordering"];
   const Array<Set<int> > domain = R.give(orbit_ordering(ordering) 
                                          ? "DOMAIN_IN_ORBIT_ORDER"
                                          : "DOMAIN");
   const Map<Set<int>,int> index_of = R.give(orbit_ordering(ordering)
                                             ? "INDEX_IN_ORBIT_ORDER_OF"
                                             : "INDEX_OF");
   const int order = R.give("GROUP.ORDER");
   const Matrix<Rational> character_table = R.give("GROUP.CHARACTER_TABLE");
   const Array<Set<Array<int> > > conjugacy_classes = R.give("GROUP.CONJUGACY_CLASSES");

   if (irred_index<0 || irred_index >= character_table.rows())
      throw std::runtime_error("The given index does not refer to an irreducible representation.");

   return isotypic_basis_impl(character_table[irred_index], InducedAction<Set<int> >(degree, domain, index_of), degree, conjugacy_classes, order);
}


IncidenceMatrix<> isotypic_supports_array(perl::Object R, const Array<Set<int> >& candidates, perl::OptionSet options)
{
   const int degree = R.give("DEGREE");
   const std::string ordering = options["domain_ordering"];
   const Array<Set<int> > domain = R.give(orbit_ordering(ordering) 
                                          ? "DOMAIN_IN_ORBIT_ORDER"
                                          : "DOMAIN");
   const Map<Set<int>,int> index_of = R.give(orbit_ordering(ordering)
                                             ? "INDEX_IN_ORBIT_ORDER_OF"
                                             : "INDEX_OF");
   const int order = R.give("GROUP.ORDER");
   const Matrix<Rational> character_table = R.give("GROUP.CHARACTER_TABLE");
   const Array<Set<Array<int> > > conjugacy_classes = R.give("GROUP.CONJUGACY_CLASSES");

   const InducedAction<Set<int> > IA(degree, domain, index_of);
   SparseMatrix<Rational> S(candidates.size(), degree);
   for (int i=0; i<candidates.size(); ++i)
      S(i, index_of[candidates[i]]) = 1;
      
   return isotypic_supports_impl(S, character_table, IA, conjugacy_classes, order, degree);
}

IncidenceMatrix<> isotypic_supports_matrix(perl::Object R, const SparseMatrix<Rational>& S, perl::OptionSet options)
{
   const int degree = R.give("DEGREE");
   const std::string ordering = options["domain_ordering"];
   const Array<Set<int> > domain = R.give(orbit_ordering(ordering) 
                                          ? "DOMAIN_IN_ORBIT_ORDER"
                                          : "DOMAIN");
   const Map<Set<int>,int> index_of = R.give(orbit_ordering(ordering)
                                             ? "INDEX_IN_ORBIT_ORDER_OF"
                                             : "INDEX_OF");
   const int order = R.give("GROUP.ORDER");
   const Matrix<Rational> character_table = R.give("GROUP.CHARACTER_TABLE");
   const Array<Set<Array<int> > > conjugacy_classes = R.give("GROUP.CONJUGACY_CLASSES");

   const InducedAction<Set<int> > IA(degree, domain, index_of);
   return isotypic_supports_impl(S, character_table, IA, conjugacy_classes, order, degree);
}

Array<int> row_support_sizes(const SparseMatrix<Rational>& S)
{
   Array<int> support_sizes(S.rows());
   for (int i=0; i<S.rows(); ++i)
      support_sizes[i] = S.row(i).size();
   return support_sizes;
}

UserFunction4perl("# @category Other"
		  "# Calculate the decomposition into irreducible components of a given representation"
		  "# @param Array<Int> the character of the given representation"
                  "# @param Group the given group"
                  "# @return Array<Int>",
                  &irreducible_decomposition, "irreducible_decomposition(Array<Int> Group)");

UserFunction4perl("# @category Other"
		  "# Calculate the representation of a group element"
		  "# @param PermutationRepresentationOnSets the representation in question"
                  "# @param Array<Int> the group element"
                  "# @return SparseMatrix",
                  &rep, "rep(PermutationRepresentationOnSets Array<Int>)");

UserFunction4perl("# @category Other"
		  "# Calculate the projector into the isotypic component given by the i-th irrep"
		  "# @param PermutationRepresentationOnSets the representation in question"
                  "# @param Int the index of the sought irrep"
                  "# @option String domain_ordering the domain ordering to use: lex (default) or orbit"
                  "# @return SparseMatrix",
                  &isotypic_projector, "isotypic_projector(PermutationRepresentationOnSets Int { domain_ordering => 'lex' })");

UserFunction4perl("# @category Other"
		  "# Calculate a basis of the isotypic component given by the i-th irrep"
		  "# @param PermutationRepresentationOnSets the representation in question"
                  "# @param Int the index of the sought irrep"
                  "# @option String domain_ordering the domain ordering to use: lex (default) or orbit"
                  "# @return SparseMatrix a matrix whose rows form a basis of the i-th irrep",
                  &isotypic_basis, "isotypic_basis(PermutationRepresentationOnSets Int { domain_ordering => 'lex' })");

UserFunction4perl("# @category Other"
		  "# For each isotypic component, which of a given array of sets are supported on it?"
		  "# @param PermutationRepresentationOnSets the representation in question"
                  "# @param Array<Set> the given array of sets"
                  "# @option String domain_ordering the domain ordering to use: lex (default) or orbit"
                  "# @return IncidenceMatrix",
                  &isotypic_supports_array, "isotypic_supports(PermutationRepresentationOnSets Array<Set> { domain_ordering => 'lex' })");

UserFunction4perl("# @category Other"
		  "# For each row of a given SparseMatrix, to which isotypic components does it have a non-zero projection?"
                  "# The columns of the SparseMatrix correspond, in order, to the sets of the representation."
		  "# @param PermutationRepresentationOnSets the representation in question"
                  "# @param SparseMatrix the given matrix"
                  "# @option String domain_ordering the domain ordering to use: lex (default) or orbit"
                  "# @return IncidenceMatrix",
                  &isotypic_supports_matrix, "isotypic_supports(PermutationRepresentationOnSets SparseMatrix { domain_ordering => 'lex' })");

UserFunction4perl("# @category Other"
		  "# How many non-zero entries are there in each row of a SparseMatrix?"
                  "# @param SparseMatrix the given matrix"
                  "# @return Array<Int>",
                  &row_support_sizes, "row_support_sizes(SparseMatrix)");

}
}


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:

