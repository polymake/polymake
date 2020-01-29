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

#include "polymake/client.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Set.h"
#include "polymake/group/orbit.h"
#include "polymake/group/induced_action.h"

namespace polymake { namespace group {

// Single permutations

template<typename DomainType>
Array<Int>
induced_permutation(const Array<Int>& perm,
                    const Array<DomainType>& domain_to_induce,
                    const hash_map<DomainType, Int>& index_of)
{
   return induced_permutation_impl<on_container>(perm, domain_to_induce.size(), entire(domain_to_induce), index_of);
}


template<typename Scalar>
Array<Array<Int>>
induced_permutations(const Array<Array<Int>>& original_gens, 
                     const Matrix<Scalar>& M,
                     const hash_map<Vector<Scalar>, Int>& index_of,
                     OptionSet options)
{
   const bool homogeneous_action = options["homogeneous_action"];
   return homogeneous_action
      ? induced_permutations_impl<on_container>         (original_gens, M.rows(), entire(rows(M)), index_of)
      : induced_permutations_impl<on_nonhomog_container>(original_gens, M.rows(), entire(rows(M)), index_of);
}

template<typename Scalar>
Array<Array<Int>>
induced_permutations(const Array<Matrix<Scalar>>& original_gens, 
                     const Matrix<Scalar>& M,
                     const hash_map<Vector<Scalar>, Int>& index_of,
                     OptionSet)
{
   return induced_permutations_impl<on_elements>(original_gens, M.rows(), entire(rows(M)), index_of);
}

Array<Array<Int>>
induced_permutations_incidence(const Array<Array<Int>>& original_gens,
                               const IncidenceMatrix<>& M,
                               const hash_map<Set<Int>, Int>& index_of,
                               OptionSet)
{
   return induced_permutations_impl<on_container>(original_gens, M.rows(), entire(rows(M)), index_of);
}      

template<typename SetType>
std::enable_if_t< !pm::isomorphic_to_container_of<SetType, Set<Int>>::value, Array<Array<Int>>>
induced_permutations(const Array<Array<Int>>& original_gens,
                     const Array<SetType>& domain_to_induce,
                     const hash_map<SetType, Int>& index_of,
                     OptionSet)
{
   return induced_permutations_impl<on_container>(original_gens, domain_to_induce.size(), entire(domain_to_induce), index_of);
}

// for once nested containers, ie, Array<Set<Set>>, where the generators permute the interior set
Array<Array<Int>>
induced_permutations_set_set(const Array<Array<Int>>& original_gens,
                             const Array<Set<Set<Int>>>& domain_to_induce,
                             const hash_map<Set<Set<Int>>, Int>& index_of_)
{
   typedef hash_map<Set<Set<Int>>, Int> MapType;
   MapType new_index_of;
   const MapType& index_of = valid_index_of(entire(domain_to_induce), index_of_, new_index_of);

   Array<Array<Int>> induced_gens(original_gens.size());
   auto iit = entire(induced_gens);

   for (const auto& g : original_gens) {
      Array<Int> induced_perm(index_of.size());
      const pm::operations::group::action<Set<Int>, on_container, Array<Int>> a(g);
      auto domain_it = domain_to_induce.begin();
      for (auto& ip: induced_perm) {
         Set<Set<Int>> image;
         for (const auto& ss: *domain_it) {
            image += a(ss);
         }
         try {
            ip = index_of[image];
         } catch (const no_match&) {
            std::ostringstream os;
            wrap(os) << "The given domain is not invariant under the permutation " << g;
            throw no_match(os.str());
         }
         ++domain_it;
      }
      *iit = induced_perm;
      ++iit;
   }
   return induced_gens;
}

      
UserFunctionTemplate4perl("# @category Symmetry"
                          "# gives the permutations that are induced on the rows of a matrix //M//"
                          "# by the action of //gens// on the columns of //M//"
                          "# @param Array<Array<Int>> gens a list of permutations"
                          "# @param Matrix M the matrix acted upon"
                          "# @option Bool homogeneous_action should the generators also act on the homogeneous column? Default False"
                          "# @return Array<Array<Int>>",
                          "induced_permutations<Scalar>(Array<Array<Int>>, Matrix<Scalar>; HashMap<Vector<Scalar>,Int>=(new HashMap<Vector<Scalar>,Int>) { homogeneous_action => 0 } )");

UserFunctionTemplate4perl("# @category Symmetry"
                          "# gives the permutations that are induced on the rows of a matrix //M//"
                          "# by the action of //gens// on the columns of //M//"
                          "# @param Array<Matrix<Scalar>> gens a list of matrices that act as generators"
                          "# @param Matrix M the matrix acted upon"
                          "# @option Bool homogeneous_action should the generators also act on the homogeneous column? Default False"
                          "# @return Array<Array<Int>>",
                          "induced_permutations<Scalar>(Array<Matrix<Scalar>>, Matrix<Scalar>; HashMap<Vector<Scalar>,Int>=(new HashMap<Vector<Scalar>,Int>) { homogeneous_action => 0 } )");
      
UserFunctionTemplate4perl("# @category Symmetry"
                          "# gives the permutations that are induced on an ordered collection //S//"
                          "# by the action of //gens// on the elements of //S//"
                          "# @param Array<Array<Int>> gens "
                          "# @param Array<DomainType> S the collection acted upon"
                          "# @return Array<Array<Int>>",
                          "induced_permutations<DomainType>(Array<Array<Int>>, Array<DomainType>; HashMap<DomainType,Int>=(new HashMap<DomainType,Int>), { homogeneous_action => 0 })");

UserFunction4perl("# @category Symmetry"
                  "# gives the permutations that are induced on the rows of an incidence matrix //M//"
                  "# by the action of //gens// on the columns of //M//"
                  "# @param Array<Array<Int>> a the permutation action"
                  "# @param IncidenceMatrix M the matrix acted upon"
                  "# @return Array<Array<Int>>",
                  &induced_permutations_incidence,
                  "induced_permutations(Array<Array<Int>>, IncidenceMatrix; HashMap<Set<Int>,Int>=(new HashMap<Set<Int>,Int>), { homogeneous_action => 0 })");

UserFunction4perl("# @category Symmetry"
                  "# gives the permutations that are induced on an Array<Set<Set>> by permuting the elements of the inner set"
                  "# @param Array<Array<Int>> gens the generators of permutation action"
                  "# @param Array<Set<Set>> domain the domain acted upon"
                  "# @return Array<Array<Int>>",
                  &induced_permutations_set_set,
                  "induced_permutations_set_set(Array<Array<Int>>, Array<Set<Set>>; HashMap<Set<Set>,Int>=(new HashMap<Set<Set>,Int>) )");
      
      
}}

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:

