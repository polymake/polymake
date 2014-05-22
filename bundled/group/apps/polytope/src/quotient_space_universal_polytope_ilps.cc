/* Copyright (c) 1997-2014
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
#include "polymake/SparseMatrix.h"
#include "polymake/SparseVector.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Map.h"
#include "polymake/Rational.h"
#include "polymake/linalg.h"
#include "polymake/common/boost_dynamic_bitset.h"
#include "polymake/group/permlib.h"
#include <iostream>
#include <fstream>
#include <sstream>

namespace polymake { namespace polytope {

typedef common::boost_dynamic_bitset SetType;
typedef Array<SetType> SimplexArray;

void print_lp(perl::Object p, perl::Object lp, const bool maximize, std::ostream& os);

namespace {


template<typename Scalar, typename SetType>
SparseVector<Scalar> compatibility_equation(const Matrix<Scalar> &V
                                            , const SetType& ridge_rep
                                            , int m
                                            , const Map<SetType, int>& index_of_facet_rep
                                            , const group::PermlibGroup& sym_group)
{
   typedef std::list<boost::shared_ptr<permlib::OrbitAsSet> > OrbitList;
   const group::PermlibGroup stab_group = sym_group.setwise_stabilizer(ridge_rep);
   const OrbitList orbit_list = permlib::orbits(*(stab_group.get_permlib_group()));
   const int n (V.rows());
   SparseVector<int> relation(m);

   for (Entire<OrbitList>::const_iterator oit = entire(orbit_list); !oit.at_end(); ++oit) {
      SetType facet(ridge_rep);
      facet.resize(n);
      facet += static_cast<int>(*((*oit)->begin()));
      if (facet.size() == ridge_rep.size() ||
          static_cast<size_t>(rank(V.minor(facet,All))) < 
          static_cast<size_t>(facet.size())) 
         continue;
      const SetType facet_rep = sym_group.lex_min_representative(facet);
      relation[index_of_facet_rep[facet_rep]] += (*oit)->size();
   }      
   return SparseVector<Scalar>(relation);
}

template<typename Scalar>
Matrix<Scalar> compatibility_equations(const Matrix<Scalar>& V
                                       , const IncidenceMatrix<>& VIF
                                       , const SimplexArray& exterior_ridge_reps
                                       , const SimplexArray& facet_reps
                                       , const group::PermlibGroup& sym_group
                                       , int order_of_identification_group)
{
   int index(-1);
   Map<SetType, int> index_of_facet_rep;
   for (Entire<SimplexArray>::const_iterator fit = entire(facet_reps); !fit.at_end(); ++fit)
      index_of_facet_rep[*fit] = ++index;
   
   const int
      m = facet_reps.size(),
      n = order_of_identification_group * m;
   ListMatrix<SparseVector<Scalar> > compat_eqs(0, n);

   for (Entire<SimplexArray>::const_iterator rit = entire(exterior_ridge_reps); !rit.at_end(); ++rit) {
      const SparseVector<Scalar> partial_rel = compatibility_equation(V, *rit, m, index_of_facet_rep, sym_group);
      for (int i=1; i<order_of_identification_group; ++i) {
         SparseVector<Scalar> rel(partial_rel);
         for (int j=1; j<order_of_identification_group; ++j)
            if (i!=j) 
               rel |= zero_vector<Scalar>(m);
            else
               rel |= -partial_rel;
         compat_eqs /= rel;
      }
   }
   return SparseMatrix<Scalar>(compat_eqs);
}

}



template <typename Scalar, typename SparseMatrixType>
perl::Object quotient_space_simplexity_ilp(int d
                                           , const Matrix<Scalar>& V
                                           , const IncidenceMatrix<>& VIF
                                           , const SimplexArray& exterior_ridge_reps
                                           , const SimplexArray& facet_reps
                                           , Scalar vol
                                           , const SparseMatrixType& cocircuit_equations
                                           , const Array<Array<int> >& symmetry_group_generators
                                           , const Array<Array<int> >& identification_group_generators
                                           , perl::OptionSet options)
{
   const group::PermlibGroup 
      sym_group(symmetry_group_generators),
      id_group (identification_group_generators);
   
   const int
      m = facet_reps.size(),
      order_of_identification_group = id_group.order(),
      n = order_of_identification_group * m;

   const SparseMatrix<Scalar> 
      Compatibility_Equations = compatibility_equations(V, VIF, exterior_ridge_reps, facet_reps, sym_group, order_of_identification_group),
      Inequalities            = zero_vector<Scalar>(n) | unit_matrix<Scalar>(n),
      Cocircuit_Equations     = Matrix<Scalar>(cocircuit_equations);

   SparseMatrix<Scalar> Blocked_Cocircuit_Equations = Cocircuit_Equations;
   for (int i=1; i<order_of_identification_group; ++i)
      Blocked_Cocircuit_Equations = diag(Blocked_Cocircuit_Equations, Cocircuit_Equations);

   Vector<Scalar> volumes(m);
   typename Vector<Scalar>::iterator vit = volumes.begin();
   for (Entire<SimplexArray>::const_iterator rit = entire(facet_reps); !rit.at_end(); ++rit, ++vit) 
      *vit = abs(det(V.minor(*rit, All)));

   Vector<Scalar> volume_vect = scalar2vector<Scalar>(-Integer::fac(d) * vol) | volumes;
   for (int i=1; i<order_of_identification_group; ++i)
      volume_vect |= volumes;

   const SparseMatrix<Scalar>
      Equations    = (zero_vector<Scalar>(Blocked_Cocircuit_Equations.rows() + Compatibility_Equations.rows()) | (Blocked_Cocircuit_Equations / Compatibility_Equations) ) 
                     / volume_vect;

   perl::Object lp(perl::ObjectType::construct<Scalar>("LinearProgram"));
   lp.attach("INTEGER_VARIABLES") << Array<bool>(n,true);
   lp.take("LINEAR_OBJECTIVE") << Vector<Scalar>(0|ones_vector<Scalar>(n));

   perl::Object q(perl::ObjectType::construct<Scalar>("Polytope"));
   q.take("FEASIBLE") << true;
   q.take("INEQUALITIES") << Inequalities;
   q.take("EQUATIONS") << Equations;
   q.take("LP") << lp;

   const std::string filename = options["filename"];

   if (filename.size()) {
      std::ofstream os(filename.c_str());
      print_lp(q, lp, false, os);
   }
   return q;
}

template <typename Scalar, typename SparseMatrixType>
Integer quotient_space_simplexity_lower_bound(int d
                                              , const Matrix<Scalar>& V
                                              , const IncidenceMatrix<>& VIF
                                              , const SimplexArray& exterior_ridge_reps
                                              , const SimplexArray& facet_reps
                                              , Scalar vol
                                              , const SparseMatrixType& cocircuit_equations
                                              , const Array<Array<int> >& symmetry_group_generators
                                              , const Array<Array<int> >& identification_group_generators
                                              , perl::OptionSet options)
{
   perl::Object q = quotient_space_simplexity_ilp(d, V, VIF, exterior_ridge_reps, facet_reps, vol, cocircuit_equations, symmetry_group_generators, identification_group_generators, options);
   const Scalar sll=q.give("LP.MINIMAL_VALUE");
   const Integer int_sll = convert_to<Integer>(sll); // rounding down
   return sll==int_sll? int_sll : int_sll+1;
}



UserFunctionTemplate4perl("# @category Triangulations, subdivisions and volume"
                          "# Set up an LP whose MINIMAL_VALUE is a lower bound for the minimal number of simplices needed to triangulate a polytope, point configuration or quotient manifold"
                          "# @param Int d the dimension of the input polytope, point configuration or quotient manifold "
                          "# @param Matrix V the input points or vertices "
                          "# @param Scalar volume the volume of the convex hull "
                          "# @param SparseMatrix cocircuit_equations the matrix of cocircuit equations "
                          "# @option filename a name for a file in .lp format to store the linear program"
                          "# @return an LP that provides a lower bound",
                          "quotient_space_simplexity_ilp<Scalar>($ Matrix<Scalar> IncidenceMatrix Array<Set> Array<Set> $ SparseMatrix Array<Array<Int>> Array<Array<Int>> { filename=>'' })");

UserFunctionTemplate4perl("# @category Triangulations, subdivisions and volume"
                          "# Calculate a lower bound for the minimal number of simplices needed to triangulate a polytope, point configuration or quotient manifold"
                          "# @param Int d the dimension of the input polytope, point configuration or quotient manifold "
                          "# @param Matrix V the input points or vertices "
                          "# @param Scalar volume the volume of the convex hull "
                          "# @param SparseMatrix cocircuit_equations the matrix of cocircuit equations "
                          "# @return the optimal value of an LP that provides a lower bound",
                          "quotient_space_simplexity_lower_bound<Scalar>($ Matrix<Scalar> IncidenceMatrix Array<Set> Array<Set> $ SparseMatrix Array<Array<Int>> Array<Array<Int>> { filename=>'' })");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:

