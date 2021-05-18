/* Copyright (c) 1997-2021
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
#include "polymake/Rational.h"
#define GMPRATIONAL
#include "polymake/polytope/cdd_interface.h"
#include "polymake/polytope/generic_convex_hull_client.h"
#include "polymake/linalg.h"

namespace polymake { namespace polytope {

// function only computes rays, object must be a polytope or known to be pointed
// input_rays/input_lineality remain untouched
template <typename Scalar>
void cdd_eliminate_redundant_points(BigObject p)
{
   cdd_interface::ConvexHullSolver<Scalar> solver;
   Matrix<Scalar> P=p.give("INPUT_RAYS");

   const bool isCone = !p.isa("Polytope");
   if (isCone && P.cols())
      P = zero_vector<Scalar>() | P;
   const auto non_red = solver.find_vertices_among_points(P);

   if (isCone) {
      p.take("RAYS") << P.minor(non_red.first, range(1, P.cols()-1));
      p.take("RAY_SEPARATORS") << non_red.second.minor(All, range(1, P.cols()-1));
   } else {
      p.take("RAYS") << P.minor(non_red.first, All);
      p.take("RAY_SEPARATORS") << non_red.second;
   }
   Matrix<Scalar> empty_lin_space(0, P.cols() - isCone);
   p.take("LINEALITY_SPACE") << empty_lin_space;	
}

// compute rays separators from rays
// FIXME: assumption: normal returned by cdd is contained in affine hull of rays. true?
template<typename Scalar>
void cdd_vertex_normals(BigObject p)
{
   cdd_interface::ConvexHullSolver<Scalar> solver;
   Matrix<Scalar> P=p.give("RAYS");
   const bool isCone = !p.isa("Polytope");
   if (isCone && P.cols())
      P = zero_vector<Scalar>() | P;
   const auto non_red = solver.find_vertices_among_points(P);
   if (isCone)
      p.take("RAY_SEPARATORS") << non_red.second.minor(All, range(1, non_red.second.cols()-1));
   else
      p.take("RAY_SEPARATORS") << non_red.second;
}

// remove redundancies from INPUT_RAYS/INPUT_LINEALITY
// move implicit linealities from INPUT_RAYS to LINEALITY_SPACE
template <typename Scalar>
void cdd_get_non_redundant_points(BigObject p, const bool isCone)
{
   cdd_interface::ConvexHullSolver<Scalar> solver;
   Matrix<Scalar> P = p.give("INPUT_RAYS"),
                  L = p.lookup("INPUT_LINEALITY");

   if (!align_matrix_column_dim(P, L, isCone))
      throw std::runtime_error("cdd_get_non_redundant_points - dimension mismatch between input properties");

   const auto PL = P / L;
   if (PL.rows() != 0) {
      const auto non_red = solver.get_non_redundant_points(P, L, isCone);
      if (isCone) {
         p.take("RAYS") << Matrix<Scalar>(PL.minor(non_red.first, range_from(1)));
         p.take("LINEALITY_SPACE") << Matrix<Scalar>(PL.minor(non_red.second, range_from(1)));
      } else {
         p.take("RAYS") << Matrix<Scalar>(PL.minor(non_red.first, All));
         p.take("LINEALITY_SPACE") << Matrix<Scalar>(PL.minor(non_red.second, All));
      }
      p.take("POINTED") << non_red.second.empty();
   } else {
      p.take("RAYS") << P.minor(All, range_from(isCone));
      p.take("LINEALITY_SPACE") << P.minor(All, range_from(isCone));
   }
}


// remove redundancies from INEQUALITIES/EQUATIONS
// move implicit linealities from INEQUALITIES to LINEAR_SPAN
template <typename Scalar>
void cdd_get_non_redundant_inequalities(BigObject p, const bool isCone)
{
   cdd_interface::ConvexHullSolver<Scalar> solver;
   Matrix<Scalar> P = p.give("INEQUALITIES"),
                  L = p.lookup("EQUATIONS");

   if (!align_matrix_column_dim(P, L, isCone))
      throw std::runtime_error("cdd_get_non_redundant_inequalities - dimension mismatch between input properties");
   
   const auto PL = P / L;
   if (PL.rows() != 0) {
      const auto non_red = solver.get_non_redundant_inequalities(P, L, isCone);
      if (isCone) {
         p.take("FACETS") << Matrix<Scalar>(PL.minor(non_red.first, range_from(1)));
         p.take("LINEAR_SPAN") << Matrix<Scalar>(PL.minor(non_red.second, range_from(1)));
      } else {
         // cdd doesn't handle the case of an empty polytope in the same way as polymake
         // so first check for an infeasible system
         if (is_zero(null_space(PL.minor(non_red.second, All)).col(0))) {
            p.take("FACETS") << Matrix<Scalar>(0, P.cols());
            p.take("AFFINE_HULL") << Matrix<Scalar>(PL.minor(basis_rows(PL), All));
         } else {
            // For some reason cdd does not return the trivial facet for
            // polytopes.  Nevertheless it may be implied by the facets
            // returned from cdd. So we check for containment, and if it isn't
            // there, we add it.
            Matrix<Scalar> F(PL.minor(non_red.first, All));
            Matrix<Scalar> tmp(F / unit_vector<Scalar>(F.cols(), 0));
            if(rank(tmp) > rank(F)){
               p.take("FACETS") << tmp;
            } else {
               p.take("FACETS") << F;
            }
            p.take("AFFINE_HULL") << Matrix<Scalar>(PL.minor(non_red.second, All));
         }
      }
   } else {
      p.take("FACETS") << P.minor(All, range_from(isCone));
      p.take("LINEAR_SPAN") << P.minor(All, range_from(isCone));
   }
}


// find implicit linealities in INPUT_RAYS and write LINEALITY_SPACE
template <typename Scalar>
void cdd_get_lineality_space(BigObject p, const bool isCone)
{
   cdd_interface::ConvexHullSolver<Scalar> solver;
   Matrix<Scalar> P = p.give("INPUT_RAYS"),
                  L = p.lookup("INPUT_LINEALITY");

   if (!align_matrix_column_dim(P, L, isCone))
      throw std::runtime_error("cdd_get_lineality_space - dimension mismatch between input properties");

   const auto PL = P / L;
   const Bitset lineality = solver.canonicalize_lineality(P, L, cdd_interface::representation::V);

   if (isCone)
      p.take("LINEALITY_SPACE") << Matrix<Scalar>(PL.minor(lineality, range_from(1)));
   else
      p.take("LINEALITY_SPACE") << Matrix<Scalar>(PL.minor(lineality, All));
   p.take("POINTED") << lineality.empty();
}

// find implicit linealities in INEQUALITIES and write LINEAR_SPAN
template <typename Scalar>
void cdd_get_linear_span(BigObject p, const bool isCone)
{
   cdd_interface::ConvexHullSolver<Scalar> solver;
   Matrix<Scalar> P = p.give("INEQUALITIES"),
                  L = p.lookup("EQUATIONS");

   if (!align_matrix_column_dim(P, L, isCone))
      throw std::runtime_error("cdd_get_linear_span - dimension mismatch between input properties");

   const auto PL = P / L;
   const Bitset lineality = solver.canonicalize_lineality(P, L, cdd_interface::representation::H);

   if (isCone) {
      p.take("LINEAR_SPAN") << Matrix<Scalar>(PL.minor(lineality, range_from(1)));
   } else {
      // cdd doesn't handle the case of an empty polytope in the same way as polymake
      // so first check for an infeasible system
      if (is_zero(null_space(PL.minor(lineality, All)).col(0)))
         p.take("AFFINE_HULL") << Matrix<Scalar>(PL.minor(basis_rows(PL), All));
      else
         p.take("AFFINE_HULL") << Matrix<Scalar>(PL.minor(lineality, All));
   }
}

FunctionTemplate4perl("cdd_get_non_redundant_points<Scalar>(Cone<Scalar>; $=true)");
FunctionTemplate4perl("cdd_get_non_redundant_points<Scalar>(Polytope<Scalar>; $=false)");
FunctionTemplate4perl("cdd_get_non_redundant_inequalities<Scalar>(Cone<Scalar>; $=true)");
FunctionTemplate4perl("cdd_get_non_redundant_inequalities<Scalar>(Polytope<Scalar>; $=false)");

FunctionTemplate4perl("cdd_get_lineality_space<Scalar>(Cone<Scalar>; $=true)");
FunctionTemplate4perl("cdd_get_lineality_space<Scalar>(Polytope<Scalar>; $=false)");
FunctionTemplate4perl("cdd_get_linear_span<Scalar>(Cone<Scalar>; $=true)");
FunctionTemplate4perl("cdd_get_linear_span<Scalar>(Polytope<Scalar>; $=false)");

FunctionTemplate4perl("cdd_eliminate_redundant_points<Scalar>(Cone<Scalar>)");
FunctionTemplate4perl("cdd_vertex_normals<Scalar>(Cone<Scalar>)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
