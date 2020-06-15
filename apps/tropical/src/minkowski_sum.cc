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
#include "polymake/TropicalNumber.h"

namespace polymake { namespace tropical {

template <typename Addition, typename Scalar>
BigObject minkowski_sum(const TropicalNumber<Addition,Scalar>& lambda, BigObject P, 
                        const TropicalNumber<Addition,Scalar>& mu, BigObject Q)
{
   using TNumber = TropicalNumber<Addition, Scalar>;
   const Matrix<TNumber> pointsP = P.give("VERTICES | POINTS"),
                         pointsQ = Q.give("VERTICES | POINTS");

   if (pointsP.cols() != pointsQ.cols())
      throw std::runtime_error("dimension mismatch");

   Matrix<TNumber> result(pointsP.rows()*pointsQ.rows(), pointsP.cols(),
                          entire(product(rows(lambda * pointsP),
                                         rows(mu *pointsQ),
                                         operations::add())));
   BigObject PQ(P.type(), "POINTS", result);
   PQ.set_description() << "Tropical Minkowski sum of " << P.name() << " and " << Q.name() << endl;
   return PQ;
}

UserFunctionTemplate4perl("# @category Producing a tropical polytope"
                          "# Produces the tropical polytope (//lambda// \\( \\otimes \\) //P//) \\( \\oplus \\) (//mu// \\( \\otimes \\) //Q//), where \\( \\otimes \\) and \\( \\oplus \\) are tropical scalar multiplication"
                          "# and tropical addition, respectively."
                          "# @param TropicalNumber<Addition,Scalar> lambda"
                          "# @param Polytope<Addition,Scalar> P"
                          "# @param TropicalNumber<Addition,Scalar> mu"
                          "# @param Polytope<Addition,Scalar> Q"
                          "# @return Polytope<Addition,Scalar>"
                          "# @example Create two tropical polytopes as tropical convex hulls of the given POINTS,"
                          "# and assign their tropical minkowsky sum to the variable $s."
                          "# > $p1 = new Polytope<Min>(POINTS=>[[0,2,0],[0,1,1],[0,0,2]]);"
                          "# > $p2 = new Polytope<Min>(POINTS=>[[0,-1,-1],[0,1,1],[0,0,-2]]);"
                          "# > $s = minkowski_sum(0, $p1, 0, $p2);",
                          "minkowski_sum<Addition,Scalar>($ Polytope<Addition,Scalar> $ Polytope<Addition,Scalar>)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
