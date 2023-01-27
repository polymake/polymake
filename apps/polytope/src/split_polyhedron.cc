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
#include "polymake/Matrix.h"
#include "polymake/SparseMatrix.h"
#include "polymake/Set.h"

namespace polymake { namespace polytope {

template<typename Scalar>
BigObject split_polyhedron(BigObject p_in)
{
   const Matrix<Scalar> vert=p_in.give("VERTICES");
   const Int n = vert.rows();  
   const Int d = vert.cols();
  
   const Matrix<Scalar> splits=p_in.give("SPLITS");
   Int n_splits = splits.rows();

   SparseMatrix<Scalar> facets(n_splits,n+1);
   for (Int j = 0; j < n_splits; ++j) {
      const Vector<Scalar> a=splits.row(j);
      Set<Int> left; //vertices of the left (>=) polytope
      for (Int k = 0; k < n; ++k) {
         const Scalar val=a*vert.row(k);
         if (val >= 0) {
            left.insert(k);
            if (val > 0)
               facets(j,k+1) = val;
         }
      }
      BigObject p_left("Polytope", mlist<Scalar>(), "VERTICES", vert.minor(left, All));
      const Vector<Scalar> left_centroid = p_left.give("CENTROID");
      const Scalar left_volume = p_left.give("VOLUME");
      facets(j, 0) = -d*left_volume*(a*left_centroid);
   }

   const Vector<Scalar> centroid = p_in.give("CENTROID");
   const Scalar volume = p_in.give("VOLUME");
   const Vector<Scalar> c = -d*volume*centroid;

   return BigObject("Polytope", mlist<Scalar>(),
                    "FACETS", facets,
                    "AFFINE_HULL", c | T(vert),
                    "CONE_DIM", (n-d)+1);
}

UserFunctionTemplate4perl("# @category Triangulations, subdivisions and volume"
                          "# Computes the split polyhedron of a full-dimensional"
                          "# polyhdron //P//."
                          "# @param Polytope P"
                          "# @return Polytope",
                          "split_polyhedron<Scalar>(Polytope<Scalar>)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
