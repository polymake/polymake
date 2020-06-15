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
#include "polymake/Array.h"
#include "polymake/Set.h"
#include "polymake/PowerSet.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/linalg.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/FacetList.h"

namespace polymake { namespace fan {

namespace {

// (Inefficiently) solve the system of linear equations X*A=B, with A and B matrices.
// This expresses the rows of B in terms of the rows of A.
template<typename E>
Matrix<E> express_in_basis(const Matrix<E>& A, const Matrix<E>& B)
{
   if (POLYMAKE_DEBUG) {
      if (A.cols() != B.cols())
         throw std::runtime_error("solve_system - incompatible matrix");
   }
   Matrix<E> res(B.rows(), A.rows());
   auto rit = entire(rows(res));
   for (auto bit = entire(rows(B)); !bit.at_end(); ++bit, ++rit) 
      *rit = lin_solve(T(A), *bit);
   return res;
}

}  // end anonymous namespace

template <typename E>
BigObject tiling_quotient(BigObject P, BigObject Q)
{
   const Int
      d = P.give("COMBINATORIAL_DIM"),
      e = Q.give("COMBINATORIAL_DIM");
   if (d!=e) throw std::runtime_error("The dimensions of P and Q must be equal");

   const Matrix<E>
      VP = P.give("VERTICES"),
      VQ = Q.give("VERTICES"),
      _L = Q.give("TILING_LATTICE.BASIS"),
      L = dehomogenize(_L);

   // express the rows of VP and VQ in terms of L
   Matrix<E>
      VPL = express_in_basis(L, dehomogenize(VP)),
      VQL = express_in_basis(L, dehomogenize(VQ));

   // center both sets of coordinates at the barycenter of Q,
   // so that the lattice points in the Minkowski sum P+Q correspond
   // to all translates of Q that intersect P
   const Vector<E> barycenter = (ones_vector<E>(VQL.rows()) * VQL)/VQL.rows();
   VPL -= repeat_row(barycenter, VPL.rows());
   VQL -= repeat_row(barycenter, VQL.rows());

   // Take the Minkowski sum of the transformed P and Q 
   BigObjectType Polytope("Polytope", mlist<E>());
   BigObject summand1(Polytope, "VERTICES", ones_vector<E>() | VPL);
   BigObject summand2(Polytope, "VERTICES", ones_vector<E>() | VQL);
   const Matrix<E> MV = call_function("polytope::minkowski_sum_vertices_fukuda", mlist<E>(), summand1, summand2);

   // Find the interior and boundary lattice points of the Minkowski sum
   BigObject M("Polytope", mlist<E>(), "VERTICES", MV);
   const Matrix<E> 
      ILP = M.give("INTERIOR_LATTICE_POINTS"),
      BLP = M.give("BOUNDARY_LATTICE_POINTS"),
      LPm = dehomogenize(ILP/BLP);

   // to prepare calculation of the cells of the complex,
   // get the facets of the transformed polytopes.
   // of course, we could calculate these directly...
   const Matrix<E> 
      FP = summand1.give("FACETS"),
      FQ = summand2.give("FACETS");

   Map<Vector<E>, Int> index_of;
   Int n = 0;

   // We will store the cells of the complex in a FacetList.
   // Initially, we reserve space for #vert Q vertices, 
   // but as interior vertices come in this number will grow.
   FacetList F(VQ.rows());

   // Now process each lattice point in turn: 
   // - translate Q by that lattice point
   // - intersect the translate with P 
   // - if the intersection is full-dimensional, store it
   ListMatrix<Vector<E> > coos(0, d+1);
   for (auto lit = entire(rows(LPm)); !lit.at_end(); ++lit) {
      // translate Q by *lit 
      Matrix<E> translated_facets(FQ);
      translated_facets.col(0) -= FQ.minor(All, range_from(1)) * (*lit);

      // intersect the translated polytope with P
      BigObject Cell("Polytope", mlist<E>(), "INEQUALITIES", translated_facets / FP);

      // only proceed with full-dimensional faces
      const Int dd = Cell.give("COMBINATORIAL_DIM");
      if (dd != d) continue;

      const Matrix<E> translated_vertices = Cell.give("VERTICES");

      // translate back, simultaneously store vertices and cell information
      Set<Int> vif;
      for (auto rit = entire(rows(translated_vertices)); !rit.at_end(); ++rit) {
         // translate back from the lattice point and undo the translation of the barycenter 
         const Vector<E> v (dehomogenize(*rit) - *lit + barycenter);
         const Vector<E> inhv = 1 | (v * L);   // ... and undo the linear transform
         if (!index_of.exists(inhv)) {          // if it's a new point
            index_of[inhv] = n++;               // ... store index data
            coos /= inhv;                       // ... and append coordinates to matrix
         }
         vif += index_of[inhv];
      }
      F.insertMax(vif);
   }

   // done
   return BigObject("PolyhedralComplex", mlist<E>(),
                    "VERTICES", coos,
                    "MAXIMAL_POLYTOPES", F);
}

UserFunctionTemplate4perl("# @category Producing a polyhedral complex"
			  "# Calculates the quotient of //P// by //Q//+L, where //Q//+L is a lattice tiling."
                          "# The result is a polytopal complex inside //Q//. "
			  "# @param Polytope P a polytope"
			  "# @param Polytope Q a polytope that tiles space"
			  "# @tparam Coord"
			  "# @return PolyhedralComplex",
			  "tiling_quotient<E>(Polytope<E>, Polytope<E>)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
