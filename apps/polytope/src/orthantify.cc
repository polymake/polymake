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
#include "polymake/Matrix.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/linalg.h"
#include "polymake/polytope/transform.h"
#include <cassert>

namespace polymake { namespace polytope {
namespace {

template <typename Matrix1, typename Matrix2, typename E>
Matrix<E>
orth_transform(const GenericMatrix<Matrix1,E>& F,   // facets thru origin vertex
               const GenericMatrix<Matrix2,E>& EQ)  // equations
{
   const int d = F.cols();
   const Set<int> b=basis_rows(F);

   assert(b.size() + EQ.rows() == d-1);

   // Return the inverse of the (dual) transformation
   // which maps the facets and equations to the facets of the positive orthant.

   return T( unit_vector<E>(d,0) /     // far hyperplane is fixed
             F.minor(b,All)      /
             EQ );
}

}

template <typename Scalar>
perl::Object orthantify(perl::Object p_in, int origin)
{
   const Matrix<Scalar> F=p_in.give("FACETS"),
                       AH=p_in.give("AFFINE_HULL");
   const IncidenceMatrix<> VIF=p_in.give("VERTICES_IN_FACETS");
   const Set<int> far_face=p_in.give("FAR_FACE");

   if (origin < 0) {            // origin vertex number not specified - take the first affine vertex
      origin= (sequence(0,VIF.cols())-far_face).front();
   } else {
      if (origin >= VIF.cols())
         throw std::runtime_error("origin vertex number out of range");

      if (far_face.contains(origin))
         throw std::runtime_error("specified origin vertex must be affine");
   }

   perl::Object p_out=transform<Scalar>(p_in, orth_transform(F.minor(VIF.col(origin),All), AH));
   p_out.set_description() << "Positive polytope transformed from " << p_in.name() << endl;

   p_out.take("POSITIVE") << true;
   return p_out;
}

UserFunctionTemplate4perl("# @category Transformations"
                          "# Make a polyhedron [[POSITIVE]]."
                          "# Apply an affine transformation to a polyhedron such that the vertex //v// is mapped"
                          "# to the origin (1,0,...,0) and as many facets through this vertex as possible are"
                          "# mapped to the bounding facets of the first orthant."
                          "# @param Polytope P"
                          "# @param Int v vertex to be moved to the origin."
                          "#   By default it is the first affine vertex of the polyhedron."
                          "# @return Polytope"
                          "# @example To orthantify the square, moving its first vertex to the origin, do this:"
                          "# > $p = orthantify(cube(2),1);"
                          "# > print $p->VERTICES;"
                          "# | 1 2 0"
                          "# | 1 0 0"
                          "# | 1 2 2"
                          "# | 1 0 2",
                          "orthantify<Scalar> (Polytope<Scalar>; $=-1)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
