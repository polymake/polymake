/* Copyright (c) 1997-2014
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
perl::Object minkowski_sum(const Scalar& lambda1, perl::Object p_in1, const Scalar& lambda2, perl::Object p_in2)
{
   perl::Object p_out(perl::ObjectType::construct<Scalar>("Polytope"));
   p_out.set_description() << "Minkowski sum of " << lambda1 <<"*"<< p_in1.name() << " and " << lambda2 << "*" << p_in2.name() << endl;

   const Matrix<Scalar> V1=p_in1.give("VERTICES | POINTS"),
                        V2=p_in2.give("VERTICES | POINTS");

   if (V1.cols() != V2.cols())
      throw std::runtime_error("dimension mismatch");

   const Set<int> rays1=far_points(V1),
                  rays2=far_points(V2);

   const Matrix<Scalar> L1=p_in1.give("LINEALITY_SPACE"),
                        L2=p_in2.give("LINEALITY_SPACE");

   const Matrix<Scalar> P= rays1.empty() && rays2.empty()
                           ? minkowski_sum(lambda1*V1,lambda2*V2)
                           : minkowski_sum(lambda1*V1.minor(~rays1,All), lambda2*V2.minor(~rays2,All)) /
                             (sign(lambda1)*V1.minor(rays1,All)) /
                             (sign(lambda2)*V2.minor(rays2,All)) ;

   const Matrix<Scalar> L=L1 / L2;

   p_out.take("INPUT_LINEALITY")<<L;
   p_out.take("POINTS") << P;
   return p_out;
}

UserFunctionTemplate4perl("# @category Producing a polytope from polytopes"
                          "# Produces the polytope //lambda//*//P1//+//mu//*//P2//, where * and + are scalar multiplication"
                          "# and Minkowski addition, respectively."
                          "# @param Scalar lambda"
                          "# @param Polytope P1"
                          "# @param Scalar mu"
                          "# @param Polytope P2"
                          "# @return Polytope",
                          "minkowski_sum<Scalar>($ Polytope<Scalar> $ Polytope<Scalar>)");

InsertEmbeddedRule("# @category Producing a polytope from polytopes\n"
                   "# Produces the Minkowski sum of //P1// and //P2//.\n"
                   "# @param Polytope P1\n"
                   "# @param Polytope P2\n"
                   "# @return Polytope\n"
                   "user_function minkowski_sum(Polytope Polytope) { minkowski_sum(1,$_[0],1,$_[1]); }\n");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
