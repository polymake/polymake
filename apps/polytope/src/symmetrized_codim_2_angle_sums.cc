/* Copyright (c) 1997-2022
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
#include "polymake/SparseVector.h"
#include "polymake/AccurateFloat.h"
#include "polymake/ListMatrix.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/linalg.h"
#include "polymake/hash_map"
#include "polymake/Set.h"
#include "polymake/PowerSet.h"
#include "polymake/group/permlib.h"
#include "polymake/common/lattice_tools.h"
#include "polymake/polytope/poly2lp.h"
#include <iostream>
#include <fstream>
#include <vector>

namespace polymake { namespace polytope {

typedef Set<Int> SetType;
      
namespace  {

const AccurateFloat pi_2 = AccurateFloat::pi()/2;      
   
template<typename Scalar>
AccurateFloat
solid_angle_over_pi_from_inhomogeneous_normal_vectors(const Vector<Scalar>& v1, const Vector<Scalar>& v2)
{
   const auto angle = acos( conv<Scalar,AccurateFloat>()( -v1*v2 ) / sqrt( conv<Scalar,AccurateFloat>()( sqr(v1)*sqr(v2) )));
   // ----------------------------------------------------^----
   // the minus sign implements subtracting the angle from pi.
   // This is necessary because the input vectors are normal vectors to the hyperplanes, 
   // but we want to return the solid angle between the hyperplanes
   return angle > pi_2
      ? 1 - angle / AccurateFloat::pi()
      : angle / AccurateFloat::pi();
}

template<typename Scalar>
AccurateFloat
total_angle_over_pi(const SetType& codim_2_rep, 
                    const Matrix<Scalar>& F, 
                    const IncidenceMatrix<>& VIF)
{
   std::vector<Int> containing_facets;
   for (Int i = 0; i < F.rows(); ++i) {
      if (1 == incl(VIF.row(i), codim_2_rep)) 
         containing_facets.push_back(i);
   }

   switch(containing_facets.size()) { // this is the codimension of the face
   case 0: return AccurateFloat(2); // * AccurateFloat::pi();
   case 1: return AccurateFloat(1); // * AccurateFloat::pi();
   case 2: return solid_angle_over_pi_from_inhomogeneous_normal_vectors(dehomogenize(F[containing_facets[0]]),
                                                                        dehomogenize(F[containing_facets[1]]));
   default: throw std::runtime_error("Did not expect a codim 2 face to be contained in more than two facets.");
   }
}

template<typename Scalar>
AccurateFloat
angle_over_pi_at_e(const Matrix<Scalar>& V, const SetType& simplex, const SetType& e)
{
   const SetType f(simplex - e);
   assert(f.size()==2);
   
   Vector<Scalar>
      n1 = null_space(V.minor(simplex - scalar2set(f.front()), All))[0],
      n2 = null_space(V.minor(simplex - scalar2set(f.back ()), All))[0];

   if (n1[0] < 0) n1.negate(); // the null_space() operation might return a negative first coefficient
   if (n2[0] < 0) n2.negate();

   return solid_angle_over_pi_from_inhomogeneous_normal_vectors(dehomogenize(n1), dehomogenize(n2));
}

}

template<typename Scalar>
SparseMatrix<AccurateFloat> 
symmetrized_codim_2_angle_sums(Int d,
                               const Matrix<Scalar>& V,
                               const Matrix<Scalar>& F,
                               const IncidenceMatrix<>& VIF,
                               const Array<Array<Int>>& generators,
                               const IncidenceMatrix<>& codim_2_reps,
                               const Array<SetType>& facet_reps) 
{
   group::PermlibGroup sym_group;
   Int group_order;
   if (generators.size()) {
      sym_group = group::PermlibGroup(generators);
      group_order = sym_group.order();
   } else {
      group_order = 1;
   }
   hash_map<SetType, Int> index_of_facet_rep;
   Int facet_index = -1;
   for (const auto& frep: facet_reps)
      index_of_facet_rep[frep] = ++facet_index;
   
   Int n_codim_2_reps = 0;
   hash_map<SetType, Int> index_of_codim_2_rep;
   std::vector<SetType> cd2_faces_on_hull;
   for (const auto& cd2rep: rows(codim_2_reps)) {
      index_of_codim_2_rep[cd2rep] = n_codim_2_reps++;

      Int ct = 0, i = 0;
      while (ct<2 && i<VIF.rows())
         if (1 == incl(VIF.row(i++), cd2rep)) ++ct;
      if (ct>=2) cd2_faces_on_hull.push_back(cd2rep);
   }

   const Int
      n_facet_reps(index_of_facet_rep.size()),
      n_hull_reps (cd2_faces_on_hull.size());

   for (const auto& frep: facet_reps) {
      for (auto eit = entire(all_subsets_of_k(frep, d-1)); !eit.at_end(); ++eit) {
         const SetType e(*eit), e_rep( 1 == group_order ? e : sym_group.lex_min_representative(e) );
         if (!index_of_codim_2_rep.exists(e_rep))
            index_of_codim_2_rep[e_rep] = n_codim_2_reps++;
      }
   }
   
   // rows and columns are indexed as indicated by the order of the dimensions. The equations are inhomogeneous
   SparseMatrix<AccurateFloat> codim_2_angle_sums(    n_codim_2_reps + n_hull_reps,
                                                  1 + n_facet_reps   + n_codim_2_reps);

   // add the dihedral angle of each ridge of a simplex to the correct entry in the matrix
   for (const auto& frep: facet_reps) {
      for (auto eit = entire(all_subsets_of_k(frep, d-1)); !eit.at_end(); ++eit) {
         const SetType e(*eit), e_rep( 1 == group_order ? e : sym_group.lex_min_representative(e) );
         codim_2_angle_sums(index_of_codim_2_rep[e_rep], 1 + index_of_facet_rep[frep]) += angle_over_pi_at_e(V, frep, e);
      }
   }

   // at each ridge, the total angle is the one dictated by the polytope
   for (const auto& kv: index_of_codim_2_rep) // (key,value) pairs, i.e., (simplex, index)
      codim_2_angle_sums(kv.second, 1 + n_facet_reps + kv.second) = - total_angle_over_pi(kv.first, F, VIF);

   // each boundary ridge, i.e., one that is intersection of two facets of the polytope, must appear
   Int i = 0;
   for (const auto& foh : cd2_faces_on_hull) {
      codim_2_angle_sums(n_codim_2_reps + i, n_facet_reps + index_of_codim_2_rep[foh]+1) = AccurateFloat(1);
      codim_2_angle_sums(n_codim_2_reps + i, 0) = AccurateFloat( 1 == group_order ? -1 : -group_order/sym_group.setwise_stabilizer(foh).order() );
      ++i;
   }

   return codim_2_angle_sums;
}


template <typename Scalar>
BigObject
simplexity_ilp_with_angles(Int d, 
                           const Matrix<Scalar>& V, 
                           const Matrix<Scalar>& F,
                           const IncidenceMatrix<>& VIF,
                           const IncidenceMatrix<>& VIR,
                           const Array<Array<Int>>& generators,
                           const Array<SetType>& facet_reps,
                           Scalar vol, 
                           const SparseMatrix<Rational>& cocircuit_equations)
{
   const auto angle_equations = symmetrized_codim_2_angle_sums(d, V, F, VIF, generators, VIR, facet_reps);

   const Int 
      cce_cols = cocircuit_equations.cols(),
      cce_rows = cocircuit_equations.rows(),
      ae_cols = angle_equations.cols()-1, // these equations are non-homogeneous
      delta_cols = ae_cols - cce_cols;
   Vector<Scalar> volume_vect(cce_cols);
   auto vit = volume_vect.begin();
   for (const auto& f: facet_reps)
      *vit = abs(det(V.minor(f, All))), ++vit;

   const SparseMatrix<Scalar> 
      Inequalities = zero_vector<Scalar>(ae_cols) | unit_matrix<Scalar>(ae_cols),
      Equations    = (zero_vector<Scalar>(cce_rows) | SparseMatrix<Scalar>(cocircuit_equations) | zero_matrix<Scalar>(cce_rows, delta_cols)) / 
                     SparseMatrix<Scalar>(angle_equations)  /
                     ((-Integer::fac(d) * vol) | volume_vect | zero_vector<Scalar>(delta_cols));

   BigObject q("Polytope", mlist<Scalar>(),
               "FEASIBLE", true,
               "EQUATIONS", Equations,
               "INEQUALITIES", Inequalities);
   BigObject lp = q.add("LP", "LINEAR_OBJECTIVE", 0 | ones_vector<Scalar>(facet_reps.size()));
   lp.attach("INTEGER_VARIABLES") << Array<bool>(ae_cols, true);
   return q;
}


UserFunctionTemplate4perl("# @category Triangulations, subdivisions and volume"
                          "# Set up an ILP whose MINIMAL_VALUE is the minimal number of simplices needed to triangulate a polytope, point configuration or quotient manifold"
                          "# @param Int d the dimension of the input polytope, point configuration or quotient manifold "
                          "# @param Matrix V the input points or vertices "
                          "# @param Matrix F the facets of the input polytope "
                          "# @param IncidenceMatrix VIF the vertices-in-facets incidence matrix "
                          "# @param IncidenceMatrix VIR the vertices-in-ridges incidence matrix "
                          "# @param Array<Array<Int>> gens the generators of the symmetry group "
                          "# @param Array<Set> MIS the (representative) maximal interior simplices "
                          "# @param Scalar volume the volume of the convex hull "
                          "# @param SparseMatrix cocircuit_equations the matrix of cocircuit equations "
                          "# @return LinearProgram an LP that provides a lower bound",
                          "simplexity_ilp_with_angles<Scalar>($ Matrix<Scalar> Matrix<Scalar> IncidenceMatrix IncidenceMatrix Array<Array<Int>> Array<Set> Scalar SparseMatrix<Scalar>)");


} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
