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
#include "polymake/Vector.h"
#include "polymake/Matrix.h"
#include "polymake/ListMatrix.h"
#include "polymake/tropical/dual_addition_version.h"
#include "polymake/TropicalNumber.h"

namespace polymake { namespace tropical {

	//Constructs the dual polynomial of hyperplanes defined by the [[POINTS]] of a cone.
	template <typename Addition, typename Scalar>
		Polynomial<TropicalNumber<typename Addition::dual,Scalar> > cone_polynomial(const Matrix<TropicalNumber<Addition,Scalar> > &points) {
			typedef TropicalNumber<typename Addition::dual, Scalar> TNumber;

			//Invert the points
			Matrix<TNumber> dual_points = 
				dual_addition_version(points,1);

			//Construct the linear polynomials
			Polynomial<TNumber> h (TNumber::one(), points.cols());
			for (auto vec=entire(rows(dual_points.top())); !vec.at_end(); ++vec) {
				h *= Polynomial<TNumber>(*vec, unit_matrix<int>(points.cols()));
			}
			
			return h;
		}

	//Constructs the dome of the above Polynomial. 
	template <typename Addition, typename Scalar>
		perl::Object dome_hyperplane_arrangement(const Matrix<TropicalNumber<Addition,Scalar> > &points) {
		
			typedef TropicalNumber<typename Addition::dual, Scalar> TNumber;

			Polynomial<TNumber> h = cone_polynomial(points);	

			const Matrix<int> monoms_int = h.monomials_as_matrix();
			Matrix<Rational> monoms(monoms_int); // cast coefficients Integer -> Rational
			const Vector< TNumber > coefs=h.coefficients_as_vector();
			const int d=monoms.cols();
			const int n=monoms.rows();

			//We have to make all exponents positive, otherwise the below equations produce
			//a wrong result. We multiply the polynomial with a single monomial, which 
			//does not change the hypersurface.
			Vector<Rational> min_degrees(monoms.cols());
			for(int v = 0; v < monoms.cols(); v++) {
				min_degrees[v] = accumulate(monoms.col(v),operations::min());
				//If the minimal degree is positive, we're good
				min_degrees[v] = std::min(min_degrees[v],Rational(0));
			}
			for(int m = 0; m < monoms.rows(); m++) {
				monoms.row(m) -= min_degrees; 
			}


			// dual to extended Newton polyhedron
			ListMatrix< Vector<Rational> > ineq;
			const TNumber zero=TNumber::zero();
			for (int i=0; i<n; ++i) {
				if (coefs[i]==zero)
					ineq /= unit_vector<Rational>(d+1,0);
				else
					ineq /= (-1)*Addition::orientation()*(Rational(coefs[i])|monoms[i]);
			}

			perl::Object dome(perl::ObjectType::construct<Scalar>("polytope::Polytope"));
			dome.take("INEQUALITIES") << ineq;
			dome.take("FEASIBLE") << true;
			dome.take("BOUNDED") << false;
			
			return dome;
		}

	FunctionTemplate4perl("cone_polynomial<Addition,Scalar>(Matrix<TropicalNumber<Addition,Scalar> >)");
	FunctionTemplate4perl("dome_hyperplane_arrangement<Addition,Scalar>(Matrix<TropicalNumber<Addition,Scalar> >)");

}}


