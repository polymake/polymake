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
#include "polymake/IncidenceMatrix.h"
#include "polymake/Set.h"
#include "polymake/group/orbit.h"
#include "polymake/group/induced_action.h"

namespace polymake { namespace group {

// Single permutations

template<typename DomainType>
Array<int>
induced_permutation(const Array<int>& perm,
                    const Array<DomainType>& domain_to_induce,
                    const hash_map<DomainType, int>& index_of)
{
   return induced_permutation_impl<on_container>(perm, domain_to_induce.size(), entire(domain_to_induce), index_of);
}




template<typename Scalar>
Array<Array<int> >
induced_permutations(const Array<Array<int> >& original_gens, 
                     const Matrix<Scalar>& M,
                     const hash_map<Vector<Scalar>, int>& index_of,
                     perl::OptionSet options)
{
   const bool homogeneous_action = options["homogeneous_action"];
   return homogeneous_action
      ? induced_permutations_impl<on_container,          Vector<Scalar> >(original_gens, M.rows(), entire(rows(M)), index_of)
      : induced_permutations_impl<on_nonhomog_container, Vector<Scalar> >(original_gens, M.rows(), entire(rows(M)), index_of);
}

Array<Array<int> >
induced_permutations_incidence(const Array<Array<int> >& original_gens,
                               const IncidenceMatrix<>& M,
                               const hash_map<Set<int>, int>& index_of,
                               perl::OptionSet)
{
   return induced_permutations_impl<on_container>(original_gens, M.rows(), entire(rows(M)), index_of);
}


template<typename SetType>
Array<Array<int> >
induced_permutations(const Array<Array<int> >& original_gens,
                     const Array<SetType>& domain_to_induce,
                     const hash_map<SetType, int>& index_of,
                     perl::OptionSet)
{
   return induced_permutations_impl<on_container>(original_gens, domain_to_induce.size(), entire(domain_to_induce), index_of);
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
                          "# gives the permutations that are induced on an ordered collection //S//"
                          "# by the action of //gens// on the elements of //S//"
                          "# @param Array<Array<Int>> gens "
                          "# @param Array<DomainType> the collection acted upon"
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

}}

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:

