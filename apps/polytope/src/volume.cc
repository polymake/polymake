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
#include "polymake/Integer.h"
#include "polymake/Array.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/linalg.h"

namespace polymake { namespace polytope {

template <typename Scalar, typename Matrix, typename Triangulation>
Scalar volume(const GenericMatrix<Matrix, Scalar>& Points, const Triangulation& tr)
{
   Scalar V(0);
   const Int dim = tr.front().size()-1;
   for (auto s=entire(tr); !s.at_end(); ++s)
      V += abs(det( Points.minor(*s,All) ));
   return V / Integer::fac(dim);
}

// The following is based on
// Lawrence: Polytope volume computation, Math. Comp. 195 (1991)
// Yet it is further specialized to the smooth case; this spares a determinant computation in the loop.
template <typename Scalar, typename Matrix1, typename Matrix2, typename IM, typename Vector>
Scalar normalized_smooth_volume(const GenericMatrix<Matrix1, Scalar>& Vertices, // (homogeneous) vertices of P = { x | Ax <= b }, which is assumed to be SMOOTH
                                const GenericMatrix<Matrix2, Scalar>& Facets,   // facet matrix (b -A), where A = primitive facet normals
                                const GenericIncidenceMatrix<IM>& FTV,          // FACETS_THRU_VERTICES
                                const GenericVector<Vector, Scalar>& obj        // sufficiently generic linear objective function (homogeneous)
                                )
// note that A is supposed to be integral, but the right hand side b may come from some funny ordered field; hence the templates
{
   const Int d = Vertices.cols()-1;
   auto V_aff = Vertices.minor(All,range(1,d)); // without homogenization
   auto A = - Facets.minor(All,range(1,d)); // outward normals
   auto c = obj.slice(range(1,d)); // without constant term

   Scalar V(0);
   for (Int i=0; i < V_aff.rows(); ++i) {
      Scalar numerator = pow(V_aff[i] * c, d);
      auto B = A.minor(FTV[i],All);
      Scalar denominator = accumulate(T(inv(B)) * c, operations::mul());
      if (denominator==0) {
         throw std::runtime_error("normalized_smooth_volume: objective function not generic enough");
      }
      V += numerator/denominator;
   }
   return V; // divide by d! to obtain Euclidean volume
}

template <typename MatrixType, typename Scalar, typename Triangulation>
Array<Scalar> squared_relative_volumes(const GenericMatrix<MatrixType, Scalar>& Points, const Triangulation& tr)
{
   Array<Scalar> V(tr.size());
   auto vit = entire(V);
   const Int dim = tr.front().size()-1;
   const Integer f = Integer::fac(dim);
   for (auto s=entire(tr); !s.at_end(); ++s, ++vit) {
      Matrix<Scalar> M(Points.minor(*s, All));
      // subtract off the first row from all the others, to place one vertex at the origin
      for (Int i = 1; i < M.rows(); ++i) 
         M.row(i) -= M.row(0);
      *vit = abs(det( M.minor(range_from(1), All) * T(M.minor(range_from(1), All)) )) / (f*f);
   }
   return V;
}

FunctionTemplate4perl("volume(Matrix *)");

FunctionTemplate4perl("normalized_smooth_volume<Scalar>(Matrix<type_upgrade<Scalar>>, Matrix<type_upgrade<Scalar>>, IncidenceMatrix, Vector<type_upgrade<Scalar>>)");

FunctionTemplate4perl("squared_relative_volumes(Matrix *)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
