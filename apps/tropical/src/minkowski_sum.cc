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
#include "polymake/linalg.h"
#include "polymake/TropicalNumber.h"

namespace polymake { namespace tropical {

	template <typename Addition, typename Scalar>
		perl::Object minkowski_sum(const TropicalNumber<Addition,Scalar>& lambda, perl::Object P, 
				const TropicalNumber<Addition,Scalar>& mu, perl::Object Q)
		{
			typedef TropicalNumber<Addition,Scalar> TNumber;
			const Matrix<TNumber> pointsP=P.give("VERTICES | POINTS"),
					pointsQ=Q.give("VERTICES | POINTS");

			if (pointsP.cols() != pointsQ.cols())
				throw std::runtime_error("dimension mismatch");

			Matrix<TNumber> result(pointsP.rows()*pointsQ.rows(), pointsP.cols(),
					entire(product(rows(lambda * pointsP),
							rows(mu *pointsQ),
							operations::add())));
			perl::Object PQ(P.type());
			PQ.set_description()<<"Tropical Minkowski sum of "<<P.name()<<" and "<<Q.name()<<endl;

			PQ.take("POINTS") << result;
			return PQ;
		}

	
		UserFunctionTemplate4perl("# @category Producing a tropical polytope"
		"# Produces the tropical polytope (//lambda// \\( \\otimes \\) //P//) \\( \\oplus \\) (//mu// \\( \\otimes \\) //Q//), where \\( \\otimes \\) and \\( \\oplus \\) are tropical scalar multiplication"
		"# and tropical addition, respectively."
		"# @param TropicalNumber<Addition,Scalar> lambda"
		"# @param Cone<Addition,Scalar> P"
		"# @param TropicalNumber<Addition,Scalar> mu"
		"# @param Cone<Addition,Scalar> Q"
		"# @return Cone<Addition,Scalar>" ,
		"minkowski_sum<Addition,Scalar>($ Cone<Addition,Scalar> $ Cone<Addition,Scalar>)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
