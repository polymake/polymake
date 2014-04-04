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

namespace polymake { namespace tropical {

template <typename Scalar>
perl::Object minkowski_sum(const Scalar& lambda, perl::Object P, const Scalar& mu, perl::Object Q)
{
   const Matrix<Scalar> pointsP=P.give("VERTICES | POINTS"),
      pointsQ=Q.give("VERTICES | POINTS");

   if (pointsP.cols() != pointsQ.cols())
      throw std::runtime_error("dimension mismatch");

   Matrix<Scalar> result(pointsP.rows()*pointsQ.rows(), pointsP.cols(),
                         entire(product(rows(pointsP+same_element_matrix(lambda,pointsP.rows(),pointsP.cols())),
                                        rows(pointsQ+same_element_matrix(mu,pointsQ.rows(),pointsQ.cols())),
                                        operations::min())));
   perl::Object PQ(perl::ObjectType::construct<Scalar>("TropicalPolytope"));
   PQ.set_description()<<"Tropical Minkowski sum of "<<P.name()<<" and "<<Q.name()<<endl;

   PQ.take("POINTS") << result;
   return PQ;
}
  
UserFunctionTemplate4perl("# @category Producing a new tropical polytope from another"
                          "# Produces the tropical polytope //lambda//*//P//+//mu//*//Q//, where * and + are tropical scalar multiplication"
                          "# and tropical addition, respectively."
                          "# @param Scalar lambda"
                          "# @param TropicalPolytope P"
                          "# @param Scalar mu" 
                          "# @param TropicalPolytope Q"
                          "# @return TropicalPolytope" ,
                          "minkowski_sum<Scalar>($ TropicalPolytope<Scalar> $ TropicalPolytope<Scalar>)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
