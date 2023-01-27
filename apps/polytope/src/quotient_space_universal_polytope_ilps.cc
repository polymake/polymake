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
#include "polymake/Array.h"
#include "polymake/Set.h"
#include "polymake/Bitset.h"
#include "polymake/SparseMatrix.h"
#include "polymake/SparseVector.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Map.h"
#include "polymake/linalg.h"
#include "polymake/group/permlib.h"
#include "polymake/polytope/poly2lp.h"
#include <iostream>
#include <fstream>
#include <sstream>

namespace polymake { namespace polytope {

namespace {   

template<typename Scalar, typename SetType>
SparseVector<Scalar>
compatibility_equation(const Matrix<Scalar> &V
                       , const SetType& ridge_rep
                       , Int m
                       , const Map<SetType, Int>& index_of_facet_rep
                       , const group::PermlibGroup& sym_group)
{
   const group::PermlibGroup stab_group = sym_group.setwise_stabilizer(ridge_rep);
   const auto orbit_list = permlib::orbits(*(stab_group.get_permlib_group()));
   const Int n = V.rows();
   SparseVector<Int> relation(m);

   for (const auto& orbit: orbit_list) {
      SetType facet(ridge_rep);
      facet.reserve(n);
      facet += static_cast<Int>(*(orbit->begin()));
      if (facet.size() == ridge_rep.size() ||
          static_cast<size_t>( rank(V.minor(facet,All)) ) < 
          static_cast<size_t>( facet.size() )) 
         continue;
      const SetType facet_rep = sym_group.lex_min_representative(facet);
      relation[index_of_facet_rep[facet_rep]] += orbit->size();
   }      
   return SparseVector<Scalar>(relation);
}

template<typename Scalar, typename SetType>
SparseMatrix<Scalar>
compatibility_equations(const Matrix<Scalar>& V
                        , const IncidenceMatrix<>& VIF
                        , const Array<SetType>& exterior_ridge_reps
                        , const Array<SetType>& facet_reps
                        , const group::PermlibGroup& sym_group
                        , Int order_of_identification_group)
{
   Int index = -1;
   Map<SetType, Int> index_of_facet_rep;
   for (const auto& f: facet_reps)
      index_of_facet_rep[f] = ++index;
   
   const Int
      m = facet_reps.size(),
      n = order_of_identification_group * m;
   ListMatrix<SparseVector<Scalar>> compat_eqs(0, n);

   for (const auto& r: exterior_ridge_reps) {
      const SparseVector<Scalar> partial_rel = compatibility_equation(V, r, m, index_of_facet_rep, sym_group);
      for (Int i = 1; i < order_of_identification_group; ++i) {
         SparseVector<Scalar> rel(partial_rel);
         for (Int j = 1; j < order_of_identification_group; ++j)
            if (i!=j) 
               rel |= zero_vector<Scalar>(m);
            else
               rel |= -partial_rel;
         compat_eqs /= rel;
      }
   }
   return SparseMatrix<Scalar>(compat_eqs);
}

} // end anonymous namespace



template <typename Scalar, typename SparseMatrixType, typename SetType>
BigObject
quotient_space_simplexity_ilp(Int d
                              , const Matrix<Scalar>& V
                              , const IncidenceMatrix<>& VIF
                              , const Array<SetType>& exterior_ridge_reps
                              , const Array<SetType>& facet_reps
                              , Scalar vol
                              , const SparseMatrixType& cocircuit_equations
                              , const Array<Array<Int>>& symmetry_group_generators
                              , const Array<Array<Int>>& identification_group_generators
                              , OptionSet options)
{
   const group::PermlibGroup 
      sym_group(symmetry_group_generators),
      id_group (identification_group_generators);
   
   const Int
      m = facet_reps.size(),
      order_of_identification_group = id_group.order(),
      n = order_of_identification_group * m;

   const SparseMatrix<Scalar> 
      Compatibility_Equations = compatibility_equations(V, VIF, exterior_ridge_reps, facet_reps, sym_group, order_of_identification_group),
      Inequalities            = zero_vector<Scalar>(n) | unit_matrix<Scalar>(n),
      Cocircuit_Equations     = Matrix<Scalar>(cocircuit_equations);

   SparseMatrix<Scalar> Blocked_Cocircuit_Equations = Cocircuit_Equations;
   for (Int i = 1; i < order_of_identification_group; ++i)
      Blocked_Cocircuit_Equations = diag(Blocked_Cocircuit_Equations, Cocircuit_Equations);

   Vector<Scalar> volumes(m);
   auto vit = volumes.begin();
   for (const auto& f: facet_reps)
      *vit = abs(det(V.minor(f, All))), ++vit;

   Vector<Scalar> volume_vect = scalar2vector<Scalar>(-Integer::fac(d) * vol) | volumes;
   for (Int i = 1; i < order_of_identification_group; ++i)
      volume_vect |= volumes;

   const SparseMatrix<Scalar>
      Equations    = (zero_vector<Scalar>(Blocked_Cocircuit_Equations.rows() + Compatibility_Equations.rows()) | (Blocked_Cocircuit_Equations / Compatibility_Equations) ) 
                     / volume_vect;

   BigObject q("Polytope", mlist<Scalar>(),
               "FEASIBLE", true,
               "INEQUALITIES", Inequalities,
               "EQUATIONS", remove_zero_rows(Equations));
   BigObject lp = q.add("LP", "LINEAR_OBJECTIVE", 0 | ones_vector<Scalar>(n));
   lp.attach("INTEGER_VARIABLES") << Array<bool>(n, true);

   const std::string filename = options["filename"];

   if (filename.size()) {
      if (filename == "-") {
         print_lp<Scalar, true>(q, lp, false, perl::cout);
      } else {
         std::ofstream os(filename.c_str());
         print_lp<Scalar, true>(q, lp, false, os);
      }
   }
   return q;
}

template <typename Scalar, typename SparseMatrixType, typename SetType>
Integer quotient_space_simplexity_lower_bound(Int d
                                              , const Matrix<Scalar>& V
                                              , const IncidenceMatrix<>& VIF
                                              , const Array<SetType>& exterior_ridge_reps
                                              , const Array<SetType>& facet_reps
                                              , Scalar vol
                                              , const SparseMatrixType& cocircuit_equations
                                              , const Array<Array<Int>>& symmetry_group_generators
                                              , const Array<Array<Int>>& identification_group_generators
                                              , OptionSet options)
{
   BigObject q = quotient_space_simplexity_ilp(d, V, VIF, exterior_ridge_reps, facet_reps, vol, cocircuit_equations, symmetry_group_generators, identification_group_generators, options);
   const Scalar sll=q.give("LP.MINIMAL_VALUE");
   const Integer int_sll(floor(sll));
   return sll==int_sll? int_sll : int_sll+1;
}



UserFunctionTemplate4perl("# @category Triangulations, subdivisions and volume"
                          "# Set up an LP whose MINIMAL_VALUE is a lower bound for the minimal number of simplices needed to triangulate a polytope, point configuration or quotient manifold"
                          "# @param Int d the dimension of the input polytope, point configuration or quotient manifold "
                          "# @param Matrix V the input points or vertices "
                          "# @param Scalar volume the volume of the convex hull "
                          "# @param SparseMatrix cocircuit_equations the matrix of cocircuit equations "
                          "# @option [complete file] String filename a name for a file in .lp format to store the linear program"
                          "# @return LinearProgram an LP that provides a lower bound",
                          "quotient_space_simplexity_ilp<Scalar,MatrixType,SetType>($ Matrix<Scalar> IncidenceMatrix Array<SetType> Array<SetType> $ MatrixType Array<Array<Int>> Array<Array<Int>> { filename=>'' })");

UserFunctionTemplate4perl("# @category Triangulations, subdivisions and volume"
                          "# Calculate a lower bound for the minimal number of simplices needed to triangulate a polytope, point configuration or quotient manifold"
                          "# @param Int d the dimension of the input polytope, point configuration or quotient manifold "
                          "# @param Matrix V the input points or vertices "
                          "# @param Scalar volume the volume of the convex hull "
                          "# @param SparseMatrix cocircuit_equations the matrix of cocircuit equations "
                          "# @return Integer the optimal value of an LP that provides a lower bound",
                          "quotient_space_simplexity_lower_bound<Scalar,SetType>($ Matrix<Scalar> IncidenceMatrix Array<SetType> Array<SetType> $ SparseMatrix Array<Array<Int>> Array<Array<Int>> { filename=>'' })");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
