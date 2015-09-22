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
#include "polymake/Array.h"
#include "polymake/Set.h"
#include "polymake/Map.h"
#include "polymake/linalg.h"
#include "polymake/group/permlib.h"
#include "polymake/common/boost_dynamic_bitset.h"

namespace polymake { namespace group {

template <typename Container>
Map<Container, int> make_index_of(const Array<Container>& A)
{
   Map<Container, int> index_of;
   int i(0);
   for (typename Entire<Array<Container> >::const_iterator ait = entire(A); !ait.at_end(); ++ait)
      index_of[*ait] = i++;
   return index_of;
}

template <typename SetType>
SetType lex_min_representative(perl::Object G, const SetType& S)
{
   const group::PermlibGroup group = group::group_from_perlgroup(G);
   const SetType R = group.lex_min_representative(S);
   return R;
}

template <typename SetType>
Array<SetType> orbit_representatives(perl::Object G, const Array<SetType>& domain)
{
   const PermlibGroup group = group::group_from_perlgroup(G);
   Set<SetType> reps;
   for (typename Entire<Array<SetType> >::const_iterator dit = entire(domain); !dit.at_end(); ++dit) {
      reps += group.lex_min_representative(*dit);
   }
   return Array<SetType>(reps.size(), entire(reps));
}

template <typename E, typename Matrix>
SparseMatrix<int> orbit_supports(perl::Object R, const GenericMatrix<Matrix, E>& M)
{
   const Array<Set<int> > domain = R.give("DOMAIN");
   const Set<int> underlying_set = R.give("UNDERLYING_SET");
   const Array<Set<int> > reps = R.give("ORBIT_REPRESENTATIVES");
   const Array<Array<int> > generators = R.give("GROUP.GENERATORS");
   const PermlibGroup G(generators);
   const Map<Set<int>, int> index_of = make_index_of(reps);
   
   SparseMatrix<int> S(M.rows(), M.cols());
   for (int i=0; i<M.rows(); ++i) {
      const Set<int> supp(support(M[i]));
      for (Entire<Set<int> >::const_iterator sit = entire(supp); !sit.at_end(); ++sit) {
         S(i,*sit) = index_of[G.lex_min_representative(domain[*sit])];
      }
   }
   return S;
}


template <typename E, typename Matrix>
Array<Set<int> > orbit_support_sets(perl::Object R, const GenericMatrix<Matrix, E>& M)
{
   const Array<Set<int> > domain = R.give("DOMAIN");
   const Array<Set<int> > reps = R.give("ORBIT_REPRESENTATIVES");
   const Array<Array<int> > generators = R.give("GROUP.GENERATORS");
   const PermlibGroup G(generators);
   const Map<Set<int>, int> index_of = make_index_of(reps);
   
   Array<Set<int> > S(M.rows());
   for (int i=0; i<M.rows(); ++i) {
      const Set<int> supp(support(M[i]));
      for (Entire<Set<int> >::const_iterator sit = entire(supp); !sit.at_end(); ++sit) {
         S[i] += index_of[G.lex_min_representative(domain[*sit])];
      }
   }
   return S;
}


UserFunctionTemplate4perl("# @category Symmetry"
                          "# Computes the lexicographically smallest representative of a given set with respect to a group"
                          "# @param Group G a symmetry group"
                          "# @param Set S a set" 
                          "# @return Set the lex-min representative of S",
                          "lex_min_representative<SetType>(group::Group SetType)");

UserFunctionTemplate4perl("# @category Symmetry"
                          "# Computes the lexicographically smallest representatives of a given array of sets with respect to a group"
                          "# @param Group G a symmetry group"
                          "# @param Array<Set> A an array of sets" 
                          "# @return Array<Set> the lex-min representatives of the members of A",
                          "orbit_representatives<SetType>(group::Group Array<SetType>)");

UserFunctionTemplate4perl("# @category Symmetry"
                          "# For each non-zero entry of a SparseMatrix whose columns are indexed by the domain of a representation,"
                          "# compute the index of the orbit representative of the columns of non-zero entries"
                          "# @param PermutationRepresentationOnSets R a representation"
                          "# @param Matrix M a matrix" 
                          "# @return SparseMatrix<Int> the indices of the orbits of the members of A",
                          "orbit_supports(PermutationRepresentationOnSets Matrix)");

UserFunctionTemplate4perl("# @category Symmetry"
                          "# For each row of a Matrix whose columns are indexed by the domain of a representation,"
                          "# collect the indices of the orbit representatives of the columns of non-zero entries"
                          "# @param PermutationRepresentationOnSets R a representation"
                          "# @param Matrix M a matrix" 
                          "# @return Array<Set<Int>> the indices of the orbits of the members of A",
                          "orbit_support_sets(PermutationRepresentationOnSets Matrix)");

} }


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
