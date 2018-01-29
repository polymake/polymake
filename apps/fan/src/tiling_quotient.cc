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
#include "polymake/Array.h"
#include "polymake/Set.h"
#include "polymake/PowerSet.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/linalg.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/FacetList.h"
#include "polymake/polytope/minkowski_sum_fukuda.h"

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
   typename Entire<Rows<Matrix<E> > >::iterator rit = entire(rows(res));
   for (typename Entire<Rows<Matrix<E> > >::const_iterator bit = entire(rows(B)); !bit.at_end(); ++bit, ++rit) 
      *rit = lin_solve(T(A), *bit);
   return res;
}

}  // end anonymous namespace

template <typename E>
perl::Object tiling_quotient(perl::Object P, perl::Object Q)
{
   const int
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
   Array<perl::Object> summands(perl::ObjectType::construct<E>("Polytope"), 2);
   summands[0].take("VERTICES") << (ones_vector<E>() | VPL);
   summands[1].take("VERTICES") << (ones_vector<E>() | VQL);
   const Matrix<E> MV = polytope::minkowski_sum_vertices_fukuda<E>(summands);

   // Find the interior and boundary lattice points of the Minkowski sum
   perl::Object M(perl::ObjectType::construct<E>("Polytope"));
   M.take("VERTICES") << MV;
   const Matrix<E> 
      ILP = M.give("INTERIOR_LATTICE_POINTS"),
      BLP = M.give("BOUNDARY_LATTICE_POINTS"),
      LPm = dehomogenize(ILP/BLP);

   // to prepare calculation of the cells of the complex,
   // get the facets of the transformed polytopes.
   // of course, we could calculate these directly...
   const Matrix<E> 
      FP = summands[0].give("FACETS"),
      FQ = summands[1].give("FACETS");

   Map<Vector<E>, int> index_of;
   int n(0);

   // We will store the cells of the complex in a FacetList.
   // Initially, we reserve space for #vert Q vertices, 
   // but as interior vertices come in this number will grow.
   FacetList F(VQ.rows());

   // Now process each lattice point in turn: 
   // - translate Q by that lattice point
   // - intersect the translate with P 
   // - if the intersection is full-dimensional, store it
   ListMatrix<Vector<E> > coos(0, d+1);
   for (typename Entire<Rows<Matrix<E> > >::const_iterator lit = entire(rows(LPm)); !lit.at_end(); ++lit) {
      // translate Q by *lit 
      Matrix<E> translated_facets(FQ);
      translated_facets.col(0) -= FQ.minor(All, ~scalar2set(0)) * (*lit);

      // intersect the translated polytope with P
      perl::Object Cell(perl::ObjectType::construct<E>("Polytope"));
      Cell.take("INEQUALITIES") << (translated_facets / FP);

      // only proceed with full-dimensional faces
      const int dd = Cell.give("COMBINATORIAL_DIM");
      if (dd != d) continue;

      const Matrix<E> translated_vertices = Cell.give("VERTICES");

      // translate back, simultaneously store vertices and cell information
      Set<int> vif;
      for (typename Entire<Rows<Matrix<E> > >::const_iterator rit = entire(rows(translated_vertices)); !rit.at_end(); ++rit) {
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
   perl::Object PS(perl::ObjectType::construct<E>("PolyhedralComplex"));
   PS.take("VERTICES") << coos;
   PS.take("MAXIMAL_POLYTOPES") << F;
   return PS;
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
