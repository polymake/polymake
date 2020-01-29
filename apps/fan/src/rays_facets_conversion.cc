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
#include "polymake/linalg.h"
#include "polymake/Matrix.h"
#include "polymake/ListMatrix.h"
#include "polymake/Vector.h"
#include "polymake/SparseMatrix.h"
#include "polymake/SparseVector.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Set.h"
#include "polymake/PowerSet.h"
#include "polymake/Map.h"
#include "polymake/polytope/convex_hull.h"

namespace polymake { namespace fan {
namespace {

template <typename Coord>
Int signCheck(const Vector<Coord>& v)
{
   Int sgn = 0;
   for (const Coord& d : v) {
      Int s = sign(d);
      if (s != 0) {
         if (s*sgn >= 0)
            sgn = s;
         else
            return 0;
      }
   }
   return sgn;
}

}

template <typename Coord>
void raysToFacetNormals(BigObject f)
{
   const Int ambientDim = f.give("FAN_AMBIENT_DIM");
   const Matrix<Coord> rays = f.give("RAYS");
   const IncidenceMatrix<> incidence = f.give("MAXIMAL_CONES");
   const Matrix<Coord> linealitySpace = f.give("LINEALITY_SPACE | INPUT_LINEALITY");
   const Matrix<Coord> linealitySpace_extended = zero_vector<Coord>() | linealitySpace;

   Int facetcounter = 0;
   Map<Vector<Coord>, Int> facetmap;
   ListMatrix<Vector<Coord>> facets(0,ambientDim);
   ListMatrix<Vector<Coord>> linearSpan(0,ambientDim);
   RestrictedSparseMatrix<Int> facetIndices(incidence.rows());
   RestrictedIncidenceMatrix<only_rows> linearSpanIndices(incidence.rows());

   Int coneNum = 0;
   Int linealityDim = rank(linealitySpace);
   Int fanDim = rank(rays/linealitySpace);

   Matrix<Coord> fanLinearSpan;
   Set<Int> fanLinearSpanIndices;

   // find linear span of the whole fan
   if (fanDim < ambientDim) {
      if (linealityDim > 0) {
         fanLinearSpan = null_space(rays/linealitySpace);
      } else {
         if (rays.rows() > 0)
            fanLinearSpan = null_space(rays);
         else
            fanLinearSpan = unit_matrix<Coord>(ambientDim);
      }

      linearSpan /= fanLinearSpan;
      fanLinearSpanIndices += sequence(0,fanLinearSpan.rows());
   }

   // iterate cones
   for (auto cone=entire(rows(incidence)); !cone.at_end(); ++cone) {
      Set<Int> coneSet(*cone);
      Matrix<Coord> coneRays = rays.minor(coneSet,All);
      if (linealityDim > 0)
         coneRays /= linealitySpace / (-linealitySpace);

      Int coneDim = rank(coneRays);
      Int diff = fanDim - coneDim;

      linearSpanIndices.row(coneNum) += fanLinearSpanIndices;
      coneRays /= fanLinearSpan;

      // add linear span of this cone if neccessary
      if (diff > 0) {
         Int linearSpanIndex = 0;
         for (auto lsrow = entire(rows(linearSpan)); !lsrow.at_end(); ++lsrow, ++linearSpanIndex) {
            if (is_zero(coneRays * (*lsrow))) {
               linearSpanIndices.row(coneNum) +=linearSpanIndex;
               coneRays /= *lsrow;
               diff--;
            }
         }
         if (diff > 0) {
            Matrix<Coord> coneLinearSpan = null_space(coneRays);
            linearSpanIndices.row(coneNum) += sequence(linearSpan.rows(), coneLinearSpan.rows());
            linearSpan /= coneLinearSpan;
            diff -= coneLinearSpan.rows();
         }
      }

      if (coneDim < 1 || coneSet.size() == 0) {
         coneNum++;
         continue;
      }

      const Matrix<Coord> cfacets =
         polytope::enumerate_facets(zero_vector<Coord>() | rays.minor(coneSet, All), linealitySpace_extended, true).first.minor(All, range_from(1));

      for (auto facet = entire(rows(cfacets)); !facet.at_end(); ++facet) {
         if (facetmap.exists(*facet)) {
            facetIndices(coneNum,facetmap[*facet]) = 1;
         } else if (facetmap.exists(-(*facet))) {
            facetIndices(coneNum,facetmap[-(*facet)]) = -1;
         } else {
            facets /= *facet;
            facetIndices(coneNum, facetcounter) = 1;
            facetmap[*facet] = facetcounter++;
         }
      }
      coneNum++;
   }

   f.take("FACET_NORMALS") << facets;
   f.take("MAXIMAL_CONES_FACETS") << SparseMatrix<Int>(std::move(facetIndices));
   f.take("LINEAR_SPAN_NORMALS") << linearSpan;
   f.take("MAXIMAL_CONES_LINEAR_SPAN_NORMALS") << IncidenceMatrix<NonSymmetric>(std::move(linearSpanIndices));
}

FunctionTemplate4perl("raysToFacetNormals<Coord> (PolyhedralFan<Coord>)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
