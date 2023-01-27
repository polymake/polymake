/* Copyright (c) 1997-2023
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
#include "polymake/AccurateFloat.h"
#include "polymake/group/representations.h"
#include "polymake/group/orbit.h"
#include "polymake/group/isotypic_components.h"
#include "polymake/linalg.h"
#include "polymake/Set.h"
#include <algorithm>
#include <sstream>

namespace polymake { namespace group {

namespace {      

template <typename E>
struct character_computation_type {
   typedef QuadraticExtension<Rational> type;
};

template <>
struct character_computation_type<double> {
   typedef double type;
};

template <typename E>
void bad_decomposition(const Vector<E>& irr_dec)
{
   std::ostringstream err;
   wrap(err) << "The irreducible decomposition was calculated to be\n" << irr_dec << "\n"
                "It should be a nonnegative integer vector. Please check if the CONJUGACY_CLASS_REPRESENTATIVES, the CHARACTER, and the columns of the CHARACTER_TABLE all correspond to each other, in the same order.";
   throw std::runtime_error(err.str());
}

Vector<Int>
check_and_round(const Vector<QuadraticExtension<Rational>>& irr_dec)
{
   Vector<Int> result(irr_dec.size());
   auto result_it = result.begin();
   for (const auto& x : irr_dec) {
      if (denominator(x.a()) != 1 || x.b() != 0 || x < 0)
         bad_decomposition(irr_dec);

      *result_it = convert_to<Int>(x);
      ++result_it;
   }
   return result;
}

Vector<Int>
check_and_round(const Vector<double>& irr_dec)
{
   Vector<Int> result(irr_dec.size());
   auto result_it = result.begin();
   for (double x : irr_dec) {
      bool is_rounded;
      AccurateFloat rounded = round_if_integer(AccurateFloat{x}, is_rounded, 1e-8);
      if (!is_rounded || rounded < 0)
         bad_decomposition(irr_dec);

      *result_it = static_cast<Int>(rounded);
      ++result_it;
   }
   return result;
}

} // end anonymous namespace
      
template <typename E>
Vector<Int>
irreducible_decomposition(const Vector<E>& character, BigObject G)
{
   const Matrix<E>   character_table = G.give("CHARACTER_TABLE");
   const Array<Int>  cc_sizes        = G.give("CONJUGACY_CLASS_SIZES");
   const Int         order           = G.give("ORDER");

   if (character.size() != character_table.cols())
      throw std::runtime_error("The given array is not of the right size to be a character of the group.");

   Vector<E> weighted_character(character);
   for (Int i = 0; i < weighted_character.size(); ++i)
      weighted_character[i] *= static_cast<E>(cc_sizes[i]);

   return check_and_round(Vector<E>(character_table * weighted_character / static_cast<E>(order)));
}


SparseMatrix<Rational>
induced_rep(BigObject cone,
            BigObject action,
            const Array<Int>& perm)
{
   const Int                     degree      = action.give("DEGREE");
   const std::string             domain_name = action.give("DOMAIN_NAME");
   const hash_map<Set<Int>, Int> index_of    = action.give("INDEX_OF");

   const Array<Set<Int>>         domain      = cone.give(domain_name);

   return InducedAction<Set<Int>>(degree, domain, index_of).induced_rep(perm);
}


template<typename Element>
Array<Int>
to_orbit_order(const Array<Element>& generators,
               const Array<Int>& orbit_representatives)
{
   Array<Int> orbit_order(generators[0].size());
   Int i = 0;
   for (const auto& orep : orbit_representatives)
      for (const auto& o : Set<Int>(entire(orbit<on_elements>(generators, orep))))
         orbit_order[o] = i++;
   return orbit_order;
}

FunctionTemplate4perl("to_orbit_order(Array<Array<Int>> Array<Int>)");


SparseMatrix<CharacterNumberType>
isotypic_projector_permutations(BigObject G,
                                BigObject A,
                                Int irred_index,
                                OptionSet options)
{
   const Matrix<CharacterNumberType> character_table = G.give("CHARACTER_TABLE");
   if (irred_index<0 || irred_index >= character_table.rows())
      throw std::runtime_error("The given index does not refer to an irreducible representation.");

   const Int                order             = G.give("ORDER");
   const ConjugacyClasses<> conjugacy_classes = A.give("CONJUGACY_CLASSES");

   const bool permute_to_orbit_order = options["permute_to_orbit_order"];
   Array<Int> permutation_to_orbit_order;
   if (permute_to_orbit_order) {
      A.give("PERMUTATION_TO_ORBIT_ORDER") >> permutation_to_orbit_order;
   } else {
      permutation_to_orbit_order = sequence(0, degree(conjugacy_classes[0][0]));
   }
   
   return isotypic_projector_impl(character_table[irred_index], conjugacy_classes, permutation_to_orbit_order, order, CharacterNumberType());
}

template<typename Scalar>
auto
isotypic_projector(BigObject G,
                   BigObject A,
                   Int irred_index,
                   OptionSet options)
{
   typedef typename character_computation_type<Scalar>::type CCT;
   const Matrix<CCT> character_table = G.give("CHARACTER_TABLE");
   if (irred_index<0 || irred_index >= character_table.rows())
      throw std::runtime_error("The given index does not refer to an irreducible representation.");

   const Int                                order             = G.give("ORDER");
   const ConjugacyClasses<Matrix<Scalar>>   conjugacy_classes = A.give("CONJUGACY_CLASSES");
   
   const bool permute_to_orbit_order = options["permute_to_orbit_order"];
   Array<Int> permutation_to_orbit_order;
   if (permute_to_orbit_order) {
      A.give("PERMUTATION_TO_ORBIT_ORDER") >> permutation_to_orbit_order;
   } else {
      permutation_to_orbit_order = sequence(0, degree(conjugacy_classes[0][0]));
   }
   
   return isotypic_projector_impl(Vector<CCT>(character_table[irred_index]), conjugacy_classes, permutation_to_orbit_order, order, Scalar());
}


SparseMatrix<CharacterNumberType>
isotypic_basis_on_sets(BigObject G,
                       BigObject R,
                       Int irred_index,
                       OptionSet options) {
   const Matrix<CharacterNumberType> character_table = G.give("CHARACTER_TABLE");
   if (irred_index<0 || irred_index >= character_table.rows())
      throw std::runtime_error("The given index does not refer to an irreducible representation.");

   const Int                order                = G.give("ORDER");
   const ConjugacyClasses<> conjugacy_classes    = R.give("CONJUGACY_CLASSES");

   const bool permute_to_orbit_order = options["permute_to_orbit_order"];
   Array<Int> permutation_to_orbit_order;
   if (permute_to_orbit_order) {
      R.give("PERMUTATION_TO_ORBIT_ORDER") >> permutation_to_orbit_order;
   } else {
      permutation_to_orbit_order = sequence(0, degree(conjugacy_classes[0][0]));
   }

   return isotypic_basis_impl(character_table[irred_index], conjugacy_classes, permutation_to_orbit_order, order);
}

SparseMatrix<CharacterNumberType>
isotypic_basis_permutations(BigObject G,
                            BigObject A,
                            Int irred_index,
                            OptionSet options) {
   const Int                         order           = G.give("ORDER");
   const Matrix<CharacterNumberType> character_table = G.give("CHARACTER_TABLE");

   if (irred_index<0 || irred_index >= character_table.rows())
      throw std::runtime_error("The given index does not refer to an irreducible representation.");

   const ConjugacyClasses<> conjugacy_classes = A.give("CONJUGACY_CLASSES");

   const bool permute_to_orbit_order = options["permute_to_orbit_order"];
   Array<Int> permutation_to_orbit_order;
   if (permute_to_orbit_order) {
      A.give("PERMUTATION_TO_ORBIT_ORDER") >> permutation_to_orbit_order;
   } else {
      permutation_to_orbit_order = sequence(0, degree(conjugacy_classes[0][0]));
   }
   
   return isotypic_basis_impl(character_table[irred_index], conjugacy_classes, permutation_to_orbit_order, order);
}

template<typename Scalar>
SparseMatrix<CharacterNumberType>
isotypic_basis(BigObject G,
               BigObject A,
               Int irred_index,
               OptionSet options) {
   const auto B = isotypic_projector<Scalar>(G, A, irred_index, options);
   return B.minor(basis_rows(B), All);
}

IncidenceMatrix<> 
isotypic_supports_array(BigObject P,
                        BigObject R,
                        const Array<Set<Int>>& candidates,
                        OptionSet options)
{
   const Int                         order                      = P.give("GROUP.ORDER");
   const Matrix<CharacterNumberType> character_table            = P.give("GROUP.CHARACTER_TABLE");
   const ConjugacyClasses<>          conjugacy_classes          = R.give("CONJUGACY_CLASSES");
   const hash_map<Set<Int>, Int>     index_of                   = R.give("INDEX_OF");

   const Int deg = degree(conjugacy_classes[0][0]);

   const bool permute_to_orbit_order = options["permute_to_orbit_order"];
   Array<Int> permutation_to_orbit_order;
   if (permute_to_orbit_order) {
      R.give("PERMUTATION_TO_ORBIT_ORDER") >> permutation_to_orbit_order;
   } else {
      permutation_to_orbit_order = sequence(0, deg);
   }


   SparseMatrix<Rational> S(candidates.size(), deg);
   for (Int i = 0; i < candidates.size(); ++i)
      S(i, permutation_to_orbit_order[index_of.at(candidates[i])]) = 1;
      
   return isotypic_supports_impl(S, character_table, conjugacy_classes, permutation_to_orbit_order, order);
}


IncidenceMatrix<> 
isotypic_supports_matrix(BigObject P,
                         BigObject R,
                         const SparseMatrix<Rational>& S,
                         OptionSet options)
{
   const Matrix<CharacterNumberType> character_table            = P.give("GROUP.CHARACTER_TABLE");
   const Int                         order                      = P.give("GROUP.ORDER");
   const ConjugacyClasses<>          conjugacy_classes          = R.give("CONJUGACY_CLASSES");
   const hash_map<Set<Int>, Int>     index_of                   = R.give("INDEX_OF");

   const bool permute_to_orbit_order = options["permute_to_orbit_order"];
   Array<Int> permutation_to_orbit_order;
   if (permute_to_orbit_order) {
      R.give("PERMUTATION_TO_ORBIT_ORDER") >> permutation_to_orbit_order;
   } else {
      permutation_to_orbit_order = sequence(0, degree(conjugacy_classes[0][0]));
   }

   return isotypic_supports_impl(S, character_table, conjugacy_classes, permutation_to_orbit_order, order);

}


Array<Int>
row_support_sizes(const SparseMatrix<Rational>& S)
{
   Array<Int> support_sizes(S.rows());
   for (Int i = 0; i < S.rows(); ++i)
      support_sizes[i] = S.row(i).size();
   return support_sizes;
}

BigObject      
regular_representation(BigObject a)
{
   const Array<Array<Int>> gens = a.give("GENERATORS");
   const Int degree = gens[0].size();
   const Array<Int> id(sequence(0, degree));

   Array<Matrix<Rational>> rgens(gens.size());
   for (Int i = 0; i < gens.size(); ++i)
      rgens[i] = permutation_matrix(gens[i], id);

   BigObject r("MatrixActionOnVectors<Rational>");
   r.take("GENERATORS") << rgens;

   Array<Matrix<Rational>> rccs;
   Array<Array<Int>> ccs;
   if (a.lookup("CONJUGACY_CLASS_REPRESENTATIVES") >> ccs) {
      rccs.resize(ccs.size());
      for (Int i = 0; i < ccs.size(); ++i)
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
                  "# | >"
                  "# | "
                  "# | "
                  "# | GENERATORS"
                  "# | <0 1 0"
                  "# | 1 0 0"
                  "# | 0 0 1"
                  "# | >"
                  "# | <1 0 0"
                  "# | 0 0 1"
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

