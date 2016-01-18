/*
	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 51 Franklin Street, Fifth Floor,
	Boston, MA  02110-1301, USA.

	---
	Copyright (C) 2011 - 2015, Simon Hampe <simon.hampe@googlemail.com>

	Functions for the basic ruleset of RationalFunction
	*/

#include "polymake/client.h"
#include "polymake/Set.h"
#include "polymake/Array.h"
#include "polymake/Matrix.h"
#include "polymake/ListMatrix.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Vector.h"
#include "polymake/Rational.h"
#include "polymake/Polynomial.h"
#include "polymake/TropicalNumber.h"
#include "polymake/tropical/specialcycles.h"
#include "polymake/tropical/refine.h"
#include "polymake/tropical/misc_tools.h"
#include "polymake/tropical/linear_algebra_tools.h"
#include "polymake/tropical/polynomial_tools.h"
#include "polymake/tropical/LoggingPrinter.h"

#ifndef POLYMAKE_ATINT_RATIONAL_FUNCTION_H
#define POLYMAKE_ATINT_RATIONAL_FUNCTION_H

namespace polymake { namespace tropical {
	template <typename Addition>
		perl::Object computePolynomialDomain(const Polynomial<TropicalNumber<Addition> > &p) {
			Matrix<Rational> monoms(p.monomials_as_matrix());
			Vector<TropicalNumber<Addition> > coefs = p.coefficients_as_vector();

			if(monoms.rows() <= 1) {
				return projective_torus<Addition>(monoms.cols()-1,0);
			}

			//FIXME Same computation as in the beginning of the hypersurface client. Refactor?

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

			ListMatrix< Vector<Rational> > ineq;
			const TropicalNumber<Addition> zero=TropicalNumber<Addition>::zero();
			for (int i=0; i< monoms.rows(); ++i) {
				if (coefs[i]==zero)
					ineq /= unit_vector<Rational>(monoms.cols()+1,0);
				else
					ineq /= Addition::orientation()*(Rational(coefs[i])|monoms[i]);
			}

			perl::Object dome(perl::ObjectType::construct<Rational>("polytope::Polytope"));
			dome.take("INEQUALITIES") << ineq;
			dome.take("FEASIBLE") << true;
			dome.take("BOUNDED") << false;

			Matrix<Rational> vertices = dome.give("VERTICES");
			Matrix<Rational> lineality = dome.give("LINEALITY_SPACE");
			IncidenceMatrix<> polytopes = dome.give("VERTICES_IN_FACETS");
			Set<int> far_face = dome.give("FAR_FACE");

			//Find and eliminate the far face - Start from the end since it seems to be always there.
			for(int r = polytopes.rows()-1; r >=0; r++) {
				if(polytopes.row(r) == far_face) {
					polytopes = polytopes.minor(~scalar2set(r),All);
					break;
				}
			}

			perl::Object domain(perl::ObjectType::construct<Addition>("Cycle"));
			domain.take("PROJECTIVE_VERTICES") << vertices;
			domain.take("MAXIMAL_POLYTOPES") << polytopes;
			domain.take("LINEALITY_SPACE") << lineality;
			return domain;
		}//END computePolynomialDomain



}}

#endif
