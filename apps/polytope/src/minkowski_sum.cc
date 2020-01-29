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

namespace polymake { namespace polytope {

template <typename E, typename Matrix1, typename Matrix2>
Matrix<E>
minkowski_sum(const GenericMatrix<Matrix1,E>& A, const GenericMatrix<Matrix2,E>& B)
{
   Matrix<E> result(A.rows()*B.rows(), A.cols(),
                    entire(product(rows(A), rows(B), operations::add())));
   result.col(0).fill(1);
   return result;
}



template <typename Scalar>
Matrix<Scalar>
minkowski_sum_client(const Scalar& lambda1, const Matrix<Scalar>& V1, const Scalar& lambda2, const Matrix<Scalar>& V2)
{
  if (V1.cols() != V2.cols())
      throw std::runtime_error("dimension mismatch");

   const Set<Int> rays1 = far_points(V1),
                  rays2 = far_points(V2);

   const Matrix<Scalar> P = rays1.empty() && rays2.empty()
                            ? minkowski_sum(lambda1*V1,lambda2*V2)
                            : minkowski_sum(lambda1*V1.minor(~rays1,All), lambda2*V2.minor(~rays2,All)) /
                              (sign(lambda1)*V1.minor(rays1,All)) /
                              (sign(lambda2)*V2.minor(rays2,All)) ;

   return P;
}

FunctionTemplate4perl("minkowski_sum_client<Scalar>(type_upgrade<Scalar>, Matrix<type_upgrade<Scalar>>, type_upgrade<Scalar>, Matrix<type_upgrade<Scalar>>)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
