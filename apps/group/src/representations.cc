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
#include "polymake/group/representations.h"
#include "polymake/group/orbit.h"
#include "polymake/group/isotypic_components.h"
#include "polymake/Set.h"

namespace polymake { namespace group {


template<typename CharacterType>
Array<int> irreducible_decomposition(const CharacterType& character, perl::Object G)
{
   const Matrix<Rational> character_table = G.give("CHARACTER_TABLE");
   const Array<int> cc_sizes = G.give("CONJUGACY_CLASS_SIZES");
   const int order = G.give("ORDER");

   if (character.size() != character_table.cols())
      throw std::runtime_error("The given array is not of the right size to be a character of the group.");

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


SparseMatrix<Rational> induced_rep(perl::Object cone, perl::Object action, const Array<int>& perm) {
   const int degree = action.give("DEGREE");
   const std::string domain_name = action.give("DOMAIN_NAME");
   const Array<Set<int>> domain = cone.give(domain_name);
   const hash_map<Set<int>, int> index_of = action.give("INDEX_OF");
   return InducedAction<Set<int>>(degree, domain, index_of).induced_rep(perm);
}


Array<int>
to_orbit_order(const Array<Array<int>>& generators,
               const Array<int>& orbit_representatives)
{
   Array<int> orbit_order(generators[0].size());
   int i(0);
   for (const auto& orep : orbit_representatives)
      for (const auto& o : orbit<on_elements>(generators, orep))
         orbit_order[o] = i++;
   return orbit_order;
}

Function4perl(&to_orbit_order, "to_orbit_order(Array<Array<Int>> Array<Int>)");


SparseMatrix<Rational>
isotypic_projector(perl::Object P,
                   perl::Object R,
                   int irred_index) 
{
   const int order = P.give("GROUP.ORDER");
   const Matrix<Rational> character_table = P.give("GROUP.CHARACTER_TABLE");
   // const std::string domain_name = R.give("DOMAIN_NAME");
   // const Array<Set<int>> domain = P.give(domain_name.c_str());

   const Array<int> permutation_to_orbit_order = R.give("PERMUTATION_TO_ORBIT_ORDER");
   const ConjugacyClasses conjugacy_classes = R.give("CONJUGACY_CLASSES");
   // const hash_map<Set<int>,int> index_of = R.give("INDEX_OF");

   if (irred_index<0 || irred_index >= character_table.rows())
      throw std::runtime_error("The given index does not refer to an irreducible representation.");

   //   return isotypic_projector_impl(character_table[irred_index], InducedAction<Set<int>>(degree, domain, index_of), degree, conjugacy_classes, order);
   return isotypic_projector_impl(character_table[irred_index], conjugacy_classes, permutation_to_orbit_order, order);
}

SparseMatrix<Rational>
isotypic_basis(perl::Object P,
               perl::Object R,
               int irred_index) {
   const int order = P.give("GROUP.ORDER");
   const Matrix<Rational> character_table = P.give("GROUP.CHARACTER_TABLE");
   // const std::string domain_name = R.give("DOMAIN_NAME");
   // const Array<Set<int>> domain = P.give(domain_name.c_str());

   const Array<int> permutation_to_orbit_order = R.give("PERMUTATION_TO_ORBIT_ORDER");
   const ConjugacyClasses conjugacy_classes = R.give("CONJUGACY_CLASSES");
   // const hash_map<Set<int>,int> index_of = R.give("INDEX_OF");

   if (irred_index<0 || irred_index >= character_table.rows())
      throw std::runtime_error("The given index does not refer to an irreducible representation.");

   //   return isotypic_basis_impl(character_table[irred_index], InducedAction<Set<int>>(degree, domain, index_of), degree, conjugacy_classes, order);
   return isotypic_basis_impl(character_table[irred_index], conjugacy_classes, permutation_to_orbit_order, order);
}


IncidenceMatrix<> 
isotypic_supports_array(perl::Object P,
                        perl::Object R,
                        const Array<Set<int>>& candidates)
{
   const int order = P.give("GROUP.ORDER");
   const Matrix<Rational> character_table = P.give("GROUP.CHARACTER_TABLE");
   // const std::string domain_name = R.give("DOMAIN_NAME");
   // const Array<Set<int>> domain = P.give(domain_name.c_str());

   const ConjugacyClasses conjugacy_classes = R.give("CONJUGACY_CLASSES");
   const Array<int> permutation_to_orbit_order = R.give("PERMUTATION_TO_ORBIT_ORDER");
   const hash_map<Set<int>,int> index_of = R.give("INDEX_OF");

   // const InducedAction<Set<int>> IA(degree, domain, index_of);
   const int degree(permutation_to_orbit_order.size());
   SparseMatrix<Rational> S(candidates.size(), degree);
   for (int i=0; i<candidates.size(); ++i)
      S(i, permutation_to_orbit_order[index_of.at(candidates[i])]) = 1;
      
   return isotypic_supports_impl(S, character_table, conjugacy_classes, permutation_to_orbit_order, order);
}


IncidenceMatrix<> 
isotypic_supports_matrix(perl::Object P,
                         perl::Object R,
                         const SparseMatrix<Rational>& S)
{
   const int order = P.give("GROUP.ORDER");
   const Matrix<Rational> character_table = P.give("GROUP.CHARACTER_TABLE");
   // const std::string domain_name = R.give("DOMAIN_NAME");
   // const Array<Set<int>> domain = P.give(domain_name.c_str());

   const ConjugacyClasses conjugacy_classes = R.give("CONJUGACY_CLASSES");
   const Array<int> permutation_to_orbit_order = R.give("PERMUTATION_TO_ORBIT_ORDER");
   const hash_map<Set<int>,int> index_of = R.give("INDEX_OF");

   // const InducedAction<Set<int>> IA(degree, domain, index_of);
   return isotypic_supports_impl(S, character_table, conjugacy_classes, permutation_to_orbit_order, order);

}

Array<int> row_support_sizes(const SparseMatrix<Rational>& S)
{
   Array<int> support_sizes(S.rows());
   for (int i=0; i<S.rows(); ++i)
      support_sizes[i] = S.row(i).size();
   return support_sizes;
}



UserFunctionTemplate4perl("# @category Other"
                          "# Calculate the decomposition into irreducible components of a given representation"
                          "# @param Array the character of the given representation"
                          "# @param Group the given group"
                          "# @return Array<Int>",
                          "irreducible_decomposition<CharacterType>(CharacterType Group)");

UserFunction4perl("# @category Other"
		  "# How many non-zero entries are there in each row of a SparseMatrix?"
                  "# @param SparseMatrix the given matrix"
                  "# @return Array<Int>",
                  &row_support_sizes, "row_support_sizes(SparseMatrix)");


InsertEmbeddedRule("REQUIRE_APPLICATION polytope\n\n");

UserFunction4perl("# @category Other"
		  "# Calculate the projection map into the isotypic component given by the i-th irrep."
                  "# The map is given by a block matrix, the rows and columns of which are indexed"
                  "# by the domain elements in order of their orbits."
		  "# @param polytope::Cone C the cone or polytope containing the action in question"
		  "# @param PermutationActionOnSets A the action in question"
                  "# @param Int i the index of the sought irrep"
                  "# @return SparseMatrix",
                  &isotypic_projector, "isotypic_projector(polytope::Cone PermutationActionOnSets Int)");

UserFunction4perl("# @category Other"
		  "# Calculate a basis of the isotypic component given by the i-th irrep"
		  "# @param PermutationActionOnSets the representation in question"
                  "# @param Int the index of the sought irrep"
                  "# @return SparseMatrix a matrix whose rows form a basis of the i-th irrep",
                  &isotypic_basis, "isotypic_basis(polytope::Cone PermutationActionOnSets Int)");


UserFunction4perl("# @category Other"
		  "# Calculate the representation of a group element"
                  "# @param polytope::Cone C the cone or polytope containing the sets acted upon"
		  "# @param PermutationActionOnSets A the action in question"
                  "# @param Array<Int> g the group element, acting on vertices"
                  "# @return SparseMatrix",
                  &induced_rep, "induced_rep(polytope::Cone PermutationActionOnSets Array<Int>)");

UserFunction4perl("# @category Other"
		  "# For each isotypic component, which of a given array of sets are supported on it?"
		  "# @param PermutationActionOnSets the representation in question"
                  "# @param Array<Set> the given array of sets"
                  "# @return IncidenceMatrix",
                  &isotypic_supports_array, "isotypic_supports(polytope::Cone PermutationActionOnSets Array<Set>)");

UserFunction4perl("# @category Other"
		  "# For each row of a given SparseMatrix, to which isotypic components does it have a non-zero projection?"
                  "# The columns of the SparseMatrix correspond, in order, to the sets of the representation."
		  "# @param PermutationActionOnSets the representation in question"
                  "# @param SparseMatrix the given matrix"
                  "# @return IncidenceMatrix",
                  &isotypic_supports_matrix, "isotypic_supports(polytope::Cone PermutationActionOnSets SparseMatrix)");

}
}


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:

