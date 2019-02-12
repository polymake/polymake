/* Copyright (c) 1997-2018
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
#include "polymake/AccurateFloat.h"
#include "polymake/group/representations.h"
#include "polymake/group/orbit.h"
#include "polymake/group/isotypic_components.h"
#include "polymake/linalg.h"
#include "polymake/Set.h"
#include <algorithm>

namespace polymake { namespace group {

namespace {      

template<typename E>
struct character_computation_type {
   typedef QuadraticExtension<Rational> type;
};

template<>
struct character_computation_type<double> {
   typedef double type;
};
   
template<typename E>
inline
Vector<int>
check_and_round(const Vector<E>& irr_dec)
{
   Vector<int> irr_dec_i (irr_dec.size());
   for (int i=0; i<irr_dec.size(); ++i) {

      if (denominator(irr_dec[i].a()) != 1 ||
          irr_dec[i].b() != 0 ||
          irr_dec[i] < 0) {
         cerr << "The irreducible decomposition was calculated to be\n" << irr_dec << endl;
         throw std::runtime_error("It should be a nonnegative integer vector. Please check if the CONJUGACY_CLASS_REPRESENTATIVES, the CHARACTER, and the columns of the CHARACTER_TABLE all correspond to each other, in the same order.");
      }

      irr_dec_i[i] = convert_to<int>(irr_dec[i]);
   }
   return irr_dec_i;
}

template<>
inline
Vector<int>
check_and_round(const Vector<double>& irr_dec)
{
   const Vector<AccurateFloat> irr_dec_af(irr_dec.size(), entire(irr_dec));
   Vector<AccurateFloat> irr_dec_rounded(irr_dec.size());
   std::transform(irr_dec_af.begin(), irr_dec_af.end(), irr_dec_rounded.begin(),
                  [](const AccurateFloat& f) -> AccurateFloat { return rounded_if_integer(f); });

   if (accumulate(attach_operation(irr_dec_af - irr_dec_rounded, operations::abs_value()), operations::max()) > 1e-8 ||
       accumulate(irr_dec_rounded, operations::min()) < 0) {
      cerr << "The irreducible decomposition was calculated to be\n" << irr_dec << endl;
      throw std::runtime_error("It should be a nonnegative integer vector. Please check if the CONJUGACY_CLASS_REPRESENTATIVES, the CHARACTER, and the columns of the CHARACTER_TABLE all correspond to each other, in the same order.");
   }

   Vector<int> irr_dec_i(irr_dec.size());
   bool was_integer;
   std::transform(irr_dec_rounded.begin(), irr_dec_rounded.end(), irr_dec_i.begin(),
                  [&](const AccurateFloat& f) -> int { return round(f, was_integer); });
   return irr_dec_i;
}

} // end anonymous namespace
      
template<typename E>
Vector<int>
irreducible_decomposition(const Vector<E>& character, perl::Object G)
{
   const Matrix<E>   character_table = G.give("CHARACTER_TABLE");
   const Array<int>  cc_sizes        = G.give("CONJUGACY_CLASS_SIZES");
   const int         order           = G.give("ORDER");

   if (character.size() != character_table.cols())
      throw std::runtime_error("The given array is not of the right size to be a character of the group.");

   Vector<E> weighted_character(character);
   for (int i=0; i<weighted_character.size(); ++i)
      weighted_character[i] *= cc_sizes[i];

   return check_and_round(Vector<E>(character_table * weighted_character / order));
}


SparseMatrix<Rational>
induced_rep(perl::Object cone,
            perl::Object action,
            const Array<int>& perm)
{
   const int                     degree      = action.give("DEGREE");
   const std::string             domain_name = action.give("DOMAIN_NAME");
   const hash_map<Set<int>, int> index_of    = action.give("INDEX_OF");

   const Array<Set<int>>         domain      = cone.give(domain_name);

   return InducedAction<Set<int>>(degree, domain, index_of).induced_rep(perm);
}


template<typename Element>
Array<int>
to_orbit_order(const Array<Element>& generators,
               const Array<int>& orbit_representatives)
{
   Array<int> orbit_order(generators[0].size());
   int i(0);
   for (const auto& orep: orbit_representatives)
      for (const auto& o: Set<int>(entire(orbit<on_elements>(generators, orep))))
         orbit_order[o] = i++;
   return orbit_order;
}

FunctionTemplate4perl("to_orbit_order(Array<Array<Int>> Array<Int>)");


SparseMatrix<CharacterNumberType>
isotypic_projector_permutations(perl::Object G,
                                perl::Object A,
                                int irred_index,
                                perl::OptionSet options)
{
   const Matrix<CharacterNumberType> character_table = G.give("CHARACTER_TABLE");
   if (irred_index<0 || irred_index >= character_table.rows())
      throw std::runtime_error("The given index does not refer to an irreducible representation.");

   const int                order             = G.give("ORDER");
   const ConjugacyClasses<> conjugacy_classes = A.give("CONJUGACY_CLASSES");

   const bool permute_to_orbit_order = options["permute_to_orbit_order"];
   Array<int> permutation_to_orbit_order;
   if (permute_to_orbit_order) {
      permutation_to_orbit_order = A.give("PERMUTATION_TO_ORBIT_ORDER");
   } else {
      permutation_to_orbit_order = sequence(0, degree(conjugacy_classes[0][0]));
   }
   
   return isotypic_projector_impl(character_table[irred_index], conjugacy_classes, permutation_to_orbit_order, order, CharacterNumberType());
}

template<typename Scalar>
auto
isotypic_projector(perl::Object G,
                   perl::Object A,
                   int irred_index,
                   perl::OptionSet options)
{
   typedef typename character_computation_type<Scalar>::type CCT;
   const Matrix<CCT> character_table = G.give("CHARACTER_TABLE");
   if (irred_index<0 || irred_index >= character_table.rows())
      throw std::runtime_error("The given index does not refer to an irreducible representation.");

   const int                                order             = G.give("ORDER");
   const ConjugacyClasses<Matrix<Scalar>>   conjugacy_classes = A.give("CONJUGACY_CLASSES");
   
   const bool permute_to_orbit_order = options["permute_to_orbit_order"];
   Array<int> permutation_to_orbit_order;
   if (permute_to_orbit_order) {
      permutation_to_orbit_order = A.give("PERMUTATION_TO_ORBIT_ORDER");
   } else {
      permutation_to_orbit_order = sequence(0, degree(conjugacy_classes[0][0]));
   }
   
   return isotypic_projector_impl(Vector<CCT>(character_table[irred_index]), conjugacy_classes, permutation_to_orbit_order, order, Scalar());
}


SparseMatrix<CharacterNumberType>
isotypic_basis_on_sets(perl::Object G,
                       perl::Object R,
                       int irred_index,
                       perl::OptionSet options) {
   const Matrix<CharacterNumberType> character_table = G.give("CHARACTER_TABLE");
   if (irred_index<0 || irred_index >= character_table.rows())
      throw std::runtime_error("The given index does not refer to an irreducible representation.");

   const int                order                = G.give("ORDER");
   const ConjugacyClasses<> conjugacy_classes    = R.give("CONJUGACY_CLASSES");

   const bool permute_to_orbit_order = options["permute_to_orbit_order"];
   Array<int> permutation_to_orbit_order;
   if (permute_to_orbit_order) {
      permutation_to_orbit_order = R.give("PERMUTATION_TO_ORBIT_ORDER");
   } else {
      permutation_to_orbit_order = sequence(0, degree(conjugacy_classes[0][0]));
   }

   return isotypic_basis_impl(character_table[irred_index], conjugacy_classes, permutation_to_orbit_order, order);
}

SparseMatrix<CharacterNumberType>
isotypic_basis_permutations(perl::Object G,
                            perl::Object A,
                            int irred_index,
                            perl::OptionSet options) {
   const int                         order           = G.give("ORDER");
   const Matrix<CharacterNumberType> character_table = G.give("CHARACTER_TABLE");

   if (irred_index<0 || irred_index >= character_table.rows())
      throw std::runtime_error("The given index does not refer to an irreducible representation.");

   const ConjugacyClasses<> conjugacy_classes = A.give("CONJUGACY_CLASSES");

   const bool permute_to_orbit_order = options["permute_to_orbit_order"];
   Array<int> permutation_to_orbit_order;
   if (permute_to_orbit_order) {
      permutation_to_orbit_order = A.give("PERMUTATION_TO_ORBIT_ORDER");
   } else {
      permutation_to_orbit_order = sequence(0, degree(conjugacy_classes[0][0]));
   }
   
   return isotypic_basis_impl(character_table[irred_index], conjugacy_classes, permutation_to_orbit_order, order);
}

template<typename Scalar>
SparseMatrix<CharacterNumberType>
isotypic_basis(perl::Object G,
               perl::Object A,
               int irred_index,
               perl::OptionSet options) {
   const auto B = isotypic_projector<Scalar>(G, A, irred_index, options);
   return B.minor(basis_rows(B), All);
}

IncidenceMatrix<> 
isotypic_supports_array(perl::Object P,
                        perl::Object R,
                        const Array<Set<int>>& candidates,
                        perl::OptionSet options)
{
   const int                         order                      = P.give("GROUP.ORDER");
   const Matrix<CharacterNumberType> character_table            = P.give("GROUP.CHARACTER_TABLE");
   const ConjugacyClasses<>        conjugacy_classes          = R.give("CONJUGACY_CLASSES");
   const hash_map<Set<int>,int>      index_of                   = R.give("INDEX_OF");

   const int deg(degree(conjugacy_classes[0][0]));

   const bool permute_to_orbit_order = options["permute_to_orbit_order"];
   Array<int> permutation_to_orbit_order;
   if (permute_to_orbit_order) {
      permutation_to_orbit_order = R.give("PERMUTATION_TO_ORBIT_ORDER");
   } else {
      permutation_to_orbit_order = sequence(0, deg);
   }


   SparseMatrix<Rational> S(candidates.size(), deg);
   for (int i=0; i<candidates.size(); ++i)
      S(i, permutation_to_orbit_order[index_of.at(candidates[i])]) = 1;
      
   return isotypic_supports_impl(S, character_table, conjugacy_classes, permutation_to_orbit_order, order);
}


IncidenceMatrix<> 
isotypic_supports_matrix(perl::Object P,
                         perl::Object R,
                         const SparseMatrix<Rational>& S,
                         perl::OptionSet options)
{
   const Matrix<CharacterNumberType> character_table            = P.give("GROUP.CHARACTER_TABLE");
   const int                         order                      = P.give("GROUP.ORDER");
   const ConjugacyClasses<>        conjugacy_classes       = R.give("CONJUGACY_CLASSES");
   const hash_map<Set<int>,int>      index_of                   = R.give("INDEX_OF");

   const bool permute_to_orbit_order = options["permute_to_orbit_order"];
   Array<int> permutation_to_orbit_order;
   if (permute_to_orbit_order) {
      permutation_to_orbit_order = R.give("PERMUTATION_TO_ORBIT_ORDER");
   } else {
      permutation_to_orbit_order = sequence(0, degree(conjugacy_classes[0][0]));
   }

   return isotypic_supports_impl(S, character_table, conjugacy_classes, permutation_to_orbit_order, order);

}


Array<int>
row_support_sizes(const SparseMatrix<Rational>& S)
{
   Array<int> support_sizes(S.rows());
   for (int i=0; i<S.rows(); ++i)
      support_sizes[i] = S.row(i).size();
   return support_sizes;
}

perl::Object      
regular_representation(perl::Object a)
{
   const Array<Array<int>> gens = a.give("GENERATORS");
   const int degree = gens[0].size();
   const Array<int> id(sequence(0, degree));

   Array<Matrix<Rational>> rgens(gens.size());
   for (int i=0; i<gens.size(); ++i)
      rgens[i] = permutation_matrix(gens[i], id);

   perl::Object r("MatrixActionOnVectors<Rational>");
   r.take("GENERATORS") << rgens;

   Array<Matrix<Rational>> rccs;
   Array<Array<int>> ccs;
   if (a.lookup("CONJUGACY_CLASS_REPRESENTATIVES") >> ccs) {
      rccs.resize(ccs.size());
      for (int i=0; i<ccs.size(); ++i)
         rccs[i] = permutation_matrix(ccs[i], id);
      r.take("CONJUGACY_CLASS_REPRESENTATIVES") << rccs;
   }
   
   return r;
}

UserFunctionTemplate4perl("# @category Symmetry"
                          "# Calculate the decomposition into irreducible components of a given representation"
                          "# @param Vector<Scalar> character the character of the given representation"
                          "# @param Group G the given group; it needs to know its CHARACTER_TABLE and CONJUGACY_CLASS_SIZES."
                          "# @tparam Scalar the number type of the character"
                          "# @return Vector<Int>"
                          "# @example Remember that in polymake, we use the terms //action// and //representation// interchangeably."
                          "# To calculate the irreducible decomposition of the vertex action of the symmetry group of the 3-cube, type"
                          "# > $g = cube_group(3); $a = $g->PERMUTATION_ACTION;"
                          "# > print irreducible_decomposition($a->CHARACTER, $g);"
                          "# | 1 0 0 1 0 0 0 0 1 1"
                          "# Thus, the action of the symmetry group on the vertices decomposes into one copy of each of the"
                          "# irreducible representations corresponding to the rows 0,3,8,9 of the character table:"
                          "# > print $g->CHARACTER_TABLE->minor([0,3,8,9],All);"
                          "# | 1 1 1 1 1 1 1 1 1 1"
                          "# | 1 1 1 -1 -1 -1 -1 1 1 -1"
                          "# | 3 1 0 -1 1 -1 0 -1 -1 3"
                          "# | 3 1 0 1 -1 1 0 -1 -1 -3"
                          "# The first entries of these rows say that //a// decomposes into two 1-dimensional irreps and two 3-dimensional ones."
                          "# This correctly brings the dimension of the representation //a// to 8, the number of vertices of the 3-cube.",
                          "irreducible_decomposition<Scalar>(Vector<Scalar> Group)");

UserFunction4perl("# @category Symmetry"
                  "# How many non-zero entries are there in each row of a SparseMatrix?"
                  "# @param SparseMatrix M the given matrix"
                  "# @return Array<Int>",
                  &row_support_sizes, "row_support_sizes(SparseMatrix)");

UserFunction4perl("# @category Symmetry"
                  "# Calculate the projection map into the isotypic component given by the i-th irrep."
                  "# The map is given by a block matrix, the rows and columns of which are indexed"
                  "# by the domain elements; by default, these are ordered by orbits."
                  "# @param Group G the acting group"
                  "# @param PermutationAction A the action in question"
                  "# @param Int i the index of the sought irrep"
                  "# @option Bool permute_to_orbit_order Should the rows and columns be ordered by orbits? Default 1"
                  "# @return SparseMatrix<QuadraticExtension> pi the matrix of the projection, the rows and columns of which are indexed"
                  "# by the domain elements; by default; these are ordered by orbits."
                  "# @example Consider the action of the symmetry group of the 3-cube on its vertices."
                  "# We first calculate its decomposition into irreducible representations via"
                  "# > $g = cube_group(3); $a = $g->PERMUTATION_ACTION;"
                  "# > print irreducible_decomposition($a->CHARACTER, $g);"
                  "# | 1 0 0 1 0 0 0 0 1 1"
                  "# We now calculate the projection matrices to the irreps number 3 and 8:"
                  "# > $p3 = isotypic_projector($g,$a,3); print $p3, \"\\n\", rank($p3);"
                  "# | 1/8 -1/8 -1/8 1/8 -1/8 1/8 1/8 -1/8"
                  "# | -1/8 1/8 1/8 -1/8 1/8 -1/8 -1/8 1/8"
                  "# | -1/8 1/8 1/8 -1/8 1/8 -1/8 -1/8 1/8"
                  "# | 1/8 -1/8 -1/8 1/8 -1/8 1/8 1/8 -1/8"
                  "# | -1/8 1/8 1/8 -1/8 1/8 -1/8 -1/8 1/8"
                  "# | 1/8 -1/8 -1/8 1/8 -1/8 1/8 1/8 -1/8"
                  "# | 1/8 -1/8 -1/8 1/8 -1/8 1/8 1/8 -1/8"
                  "# | -1/8 1/8 1/8 -1/8 1/8 -1/8 -1/8 1/8"
                  "# | "
                  "# | 1"
                  "# > $p8 = isotypic_projector($g,$a,8); print $p8, \"\\n\", rank($p8);"
                  "# | 3/8 -1/8 -1/8 -1/8 -1/8 -1/8 -1/8 3/8"
                  "# | -1/8 3/8 -1/8 -1/8 -1/8 -1/8 3/8 -1/8"
                  "# | -1/8 -1/8 3/8 -1/8 -1/8 3/8 -1/8 -1/8"
                  "# | -1/8 -1/8 -1/8 3/8 3/8 -1/8 -1/8 -1/8"
                  "# | -1/8 -1/8 -1/8 3/8 3/8 -1/8 -1/8 -1/8"
                  "# | -1/8 -1/8 3/8 -1/8 -1/8 3/8 -1/8 -1/8"
                  "# | -1/8 3/8 -1/8 -1/8 -1/8 -1/8 3/8 -1/8"
                  "# | 3/8 -1/8 -1/8 -1/8 -1/8 -1/8 -1/8 3/8"
                  "# | "
                  "# | 3"
                  "# From this we deduce that the irrep indexed 3 has dimension 1, and the one indexed 8 has dimension 3."
                  "# This is consistent with the information collected in the character table:"
                  "# > print $g->CHARACTER_TABLE->minor([3,8],All);"
                  "# | 1 1 1 -1 -1 -1 -1 1 1 -1"
                  "# | 3 1 0 -1 1 -1 0 -1 -1 3"
                  "# In effect, the first entries in these rows are 1 and 3, respectively.",
                  &isotypic_projector_permutations, "isotypic_projector(Group PermutationAction Int; { permute_to_orbit_order => 1 })");

UserFunction4perl("# @category Symmetry"
                  "# Calculate the regular representation of a permutation action"
                  "# @param PermutationAction a a permutation action"
                  "# @return MatrixActionOnVectors g the regular representation of //a// by permutation matrices"
                  "# @example To calculate the regular representation of the symmetric group S_3, type"
                  "# > $s = symmetric_group(3); $s->REGULAR_REPRESENTATION;"
                  "# > print $s->REGULAR_REPRESENTATION->properties();"
                  "# | type: MatrixActionOnVectors<Rational>"
                  "# | "
                  "# | GENERATORS"
                  "# | <0 1 0"
                  "# | 1 0 0"
                  "# | 0 0 1"
                  "# | >"
                  "# | <1 0 0"
                  "# | 0 0 1"
                  "# | 0 1 0"
                  "# | >"
                  "# | "
                  "# | "
                  "# | CONJUGACY_CLASS_REPRESENTATIVES"
                  "# | <1 0 0"
                  "# | 0 1 0"
                  "# | 0 0 1"
                  "# | >"
                  "# | <0 1 0"
                  "# | 1 0 0"
                  "# | 0 0 1"
                  "# | >"
                  "# | <0 0 1"
                  "# | 1 0 0"
                  "# | 0 1 0"
                  "# | >",
                  &regular_representation, "regular_representation(PermutationAction)");

UserFunctionTemplate4perl("# @category Symmetry"
                          "# Calculate the projection map into the isotypic component given by the i-th irrep."
                          "# Note that the option //permute_to_orbit_order// makes no sense for matrix actions, so it is always set to 0."
                          "# @param Group G the acting group"
                          "# @param MatrixActionOnVectors<Scalar> A the action in question"
                          "# @param Int i the index of the sought irrep"
                          "# @tparam Scalar S the underlying number type"
                          "# @return SparseMatrix<QuadraticExtension> pi the matrix of the projection"
                          "# @example We first construct a matrix action:"
                          "# > $s = symmetric_group(3); $a = $s->REGULAR_REPRESENTATION;"
                          "# > print irreducible_decomposition($a->CHARACTER, $s);"
                          "# | 0 1 1"
                          "# Since we now know that the irreps indexed 1 and 2 appear in the regular representation, we project to one of them:"
                          "# > print isotypic_projector($s, $a, 1);"
                          "# | 2/3 -1/3 -1/3"
                          "# | -1/3 2/3 -1/3"
                          "# | -1/3 -1/3 2/3",
                          "isotypic_projector<Scalar>(Group MatrixActionOnVectors<Scalar> Int; { permute_to_orbit_order => 0 })");

UserFunction4perl("# @category Symmetry"
                  "# Calculate a basis of the isotypic component given by the i-th irrep"
                  "# @param Group G the acting group, which needs to know its CHARACTER_TABLE"
                  "# @param PermutationActionOnSets A the representation in question, which needs to know its corresponding CONJUGACY_CLASSES"
                  "# @param Int i the index of the sought irrep"
                  "# @option Bool permute_to_orbit_order Should the rows and columns be ordered by orbits? Default 1"
                  "# @return SparseMatrix a matrix whose rows form a basis of the i-th irrep"
                  "# @example Consider the action of the symmetry group of the 3-cube on the set of facets:"
                  "# > $g = cube_group(3);"
                  "# > $f = new Array<Set>([[0,2,4,6],[1,3,5,7],[0,1,4,5],[2,3,6,7],[0,1,2,3],[4,5,6,7]]);"
                  "# > $a = induced_action($g->PERMUTATION_ACTION, $f);"
                  "# > print irreducible_decomposition($a->CHARACTER, $g)"
                  "# | 1 0 0 0 0 1 0 0 0 1"
                  "# Now we can calculate a basis of the 5th irrep:"
                  "# > print isotypic_basis($g, $a, 5);"
                  "# | -1/6 -1/6 1/3 1/3 -1/6 -1/6"
                  "# | 1/3 1/3 -1/6 -1/6 -1/6 -1/6",
                  &isotypic_basis_on_sets, "isotypic_basis(Group PermutationActionOnSets Int; { permute_to_orbit_order => 1 })");

UserFunction4perl("# @category Symmetry"
                  "# Calculate a basis of the isotypic component given by the i-th irrep"
                  "# @param Group G the acting group, which needs to know its CHARACTER_TABLE"
                  "# @param PermutationAction A the action in question, which needs to know its corresponding CONJUGACY_CLASSES"
                  "# @param Int i the index of the sought irrep"
                  "# @option Bool permute_to_orbit_order Should the rows and columns be ordered by orbits? Default 1"
                  "# @return SparseMatrix a matrix whose rows form a basis of the i-th irrep"
                  "# @example Consider the action of the symmetry group of the 3-cube on its vertices."
                  "# We first calculate its decomposition into irreducible representations via"
                  "# > $g = cube_group(3); $a = $g->PERMUTATION_ACTION;"
                  "# > print irreducible_decomposition($a->CHARACTER, $g);"
                  "# | 1 0 0 1 0 0 0 0 1 1"
                  "# We now calculate a basis of the 3rd irrep:"
                  "# > print isotypic_basis($g,$a,3);"
                  "# | 1/8 -1/8 -1/8 1/8 -1/8 1/8 1/8 -1/8",
                  &isotypic_basis_permutations, "isotypic_basis(Group PermutationAction Int; { permute_to_orbit_order => 1 })");

UserFunctionTemplate4perl("# @category Symmetry"
                          "# Calculate a basis of the isotypic component given by the i-th irrep"
                          "# @param Group G the acting group, which needs to know its CHARACTER_TABLE"
                          "# @param MatrixActionOnVectors<Scalar> A the matrix action in question, which needs to know its corresponding CONJUGACY_CLASSES"
                          "# @param Int i the index of the sought irrep"
                          "# @tparam Scalar S the underlying number type"
                          "# @return SparseMatrix a matrix whose rows form a basis of the i-th irrep"
                          "# > $s = symmetric_group(3); $a = $s->REGULAR_REPRESENTATION;"
                          "# > print irreducible_decomposition($a->CHARACTER, $s);"
                          "# | 0 1 1"
                          "# We now calculate a basis of the 1st irrep:"
                          "# > print isotypic_basis($s, $a, 1);"
                          "# | 2/3 -1/3 -1/3"
                          "# | -1/3 2/3 -1/3"
                          "# This is consistent with the information collected in the character table:"
                          "# > print $g->CHARACTER_TABLE->row(1);"
                          "# | 2 0 1"
                          "# In effect, the first entry in this rows says that the dimension of this irrep is 2.",
                          "isotypic_basis<Scalar>(Group MatrixActionOnVectors<Scalar> Int; { permute_to_orbit_order => 0 })");

InsertEmbeddedRule("REQUIRE_APPLICATION polytope\n\n");

UserFunction4perl("# @category Symmetry"
                  "# For each isotypic component of a representation //a//, which of a given array //A// of sets are supported on it?"
                  "# @param PermutationActionOnSets a the representation in question"
                  "# @param Array<Set> A the given array of sets"
                  "# @option Bool permute_to_orbit_order Should the columns be ordered by orbits? Default 1"
                  "# @return IncidenceMatrix",
                  &isotypic_supports_array, "isotypic_supports(polytope::Cone PermutationActionOnSets Array<Set>; { permute_to_orbit_order => 1 })");

UserFunction4perl("# @category Symmetry"
                  "# For each row of a given SparseMatrix //M//, to which isotypic components of a representation //a// does it have a non-zero projection?"
                  "# The columns of the SparseMatrix correspond, in order, to the sets of the representation."
                  "# @param PermutationActionOnSets a the representation in question"
                  "# @param SparseMatrix M the given matrix"
                  "# @option Bool permute_to_orbit_order Should the columns be ordered by orbits? Default 1"
                  "# @return IncidenceMatrix",
                  &isotypic_supports_matrix, "isotypic_supports(polytope::Cone PermutationActionOnSets SparseMatrix; { permute_to_orbit_order => 1 })");


UserFunction4perl("# @category Symmetry"
                  "# Calculate the representation of a group element"
                  "# @param polytope::Cone C the cone or polytope containing the sets acted upon"
                  "# @param PermutationActionOnSets A the action in question"
                  "# @param Array<Int> g the group element, acting on vertices"
                  "# @return SparseMatrix",
                  &induced_rep, "induced_rep(polytope::Cone PermutationActionOnSets Array<Int>)");



}
}


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:

