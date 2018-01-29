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
#include "polymake/Matrix.h"
#include "polymake/SparseMatrix.h"
#include "polymake/Set.h"

namespace polymake { namespace polytope {

template<typename Scalar>
perl::Object split_polyhedron(perl::Object p_in)
{
   const Matrix<Scalar> vert=p_in.give("VERTICES");
   const int n=vert.rows();  
   const int d=vert.cols();
  
   const Matrix<Scalar> splits=p_in.give("SPLITS");
   int n_splits=splits.rows();

   SparseMatrix<Scalar> facets(n_splits,n+1);
   for (int j=0; j<n_splits; ++j) {
      const Vector<Scalar> a=splits.row(j);
      Set<int> left; //vertices of the left (>=) polytope
      for (int k=0; k<n; ++k) {
         const Scalar val=a*vert.row(k);
         if (val>=0) {
            left.insert(k);
            if (val>0) facets(j,k+1)=val;
         }
      }
      perl::Object p_left(perl::ObjectType::construct<Scalar>("Polytope"));
      p_left.take("VERTICES")<<vert.minor(left,All);
      const Vector<Scalar> left_centroid=p_left.give("CENTROID");
      const Scalar left_volume=p_left.give("VOLUME");
      facets(j,0)=-d*left_volume*(a*left_centroid);
   }
   perl::Object p_out(perl::ObjectType::construct<Scalar>("Polytope"));
   p_out.take("FACETS")<<facets;

   const Vector<Scalar> centroid=p_in.give("CENTROID");
   const Scalar volume=p_in.give("VOLUME");
   const Vector<Scalar> c=-d*volume*centroid;
   p_out.take("AFFINE_HULL")<<(c|T(vert));
   p_out.take("CONE_DIM")<<(n-d)+1;

   return p_out;
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
