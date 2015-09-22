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
#include "polymake/tropical/rational_function.h"

namespace polymake { namespace tropical {

	typedef std::pair<Vector<Rational>, Vector<Rational> > ValuePair;


	/**
	 * @brief Computes the property [[DOMAIN]] from [[NUMERATOR]] and [[DENOMINATOR]].
	 */
	template <typename Addition>
		void computeDomain(perl::Object function) {
			Polynomial<TropicalNumber<Addition> > num = function.give("NUMERATOR");
			Polynomial<TropicalNumber<Addition> > den = function.give("DENOMINATOR");

			perl::Object domain_num = computePolynomialDomain(num);
			perl::Object domain_den = computePolynomialDomain(den);

			RefinementResult r = refinement(domain_num, domain_den, false,false,false,true,false);
			function.take("DOMAIN") << r.complex;
		}//END computeDomain


	/**
	 * @brief Computes properties [[VERTEX_VALUES]] and [[LINEALITY_VALUES]]
	 * from [[[DOMAIN]], [NUMERATOR]] and [[DENOMINATOR]].
	 */
	template <typename Addition>
		void computeGeometricFunctionData(perl::Object function) {
			Polynomial<TropicalNumber<Addition> > num = function.give("NUMERATOR");
			Polynomial<TropicalNumber<Addition> > den = function.give("DENOMINATOR");
			perl::Object domain = function.give("DOMAIN");

			//This just computes the associated vertices
			RefinementResult r = refinement(domain, domain, false,false,true,false,false);

			Matrix<Rational> separated_vertices = domain.give("SEPARATED_VERTICES");
			std::pair<Set<int>,Set<int> > vertex_list = far_and_nonfar_vertices(separated_vertices);
			separated_vertices = separated_vertices.minor(All,~scalar2set(0));
			Matrix<Rational> lineality = domain.give("LINEALITY_SPACE");
			lineality = lineality.minor(All,~scalar2set(0));
			Vector<int> assocRep = r.associatedRep;

			Vector<Rational> vertexValues(separated_vertices.rows());
			Vector<Rational> linealityValues(lineality.rows());


			//Compute values for all nonfar vertices
			for(Entire<Set<int> >::iterator v = entire(vertex_list.second); !v.at_end(); v++) {
				vertexValues[*v] = evaluate_polynomial(num, separated_vertices.row(*v)) -
					evaluate_polynomial(den, separated_vertices.row(*v));
			}

			//For all far vertices, compute slope with respect to associated vertex
			for(Entire<Set<int> >::iterator r= entire(vertex_list.first); !r.at_end(); r++) {
				Vector<Rational> associated_vertex = separated_vertices.row(assocRep[*r]);
				vertexValues[*r] = ( evaluate_polynomial(num, associated_vertex + separated_vertices.row(*r)) -
						evaluate_polynomial(den, associated_vertex + separated_vertices.row(*r))) -
					vertexValues[assocRep[*r]];
			}

			function.take("VERTEX_VALUES") << vertexValues;

			//Same for lineality space generators - we use a fixed vertex as base vertex
			int base_vertex_index = *(vertex_list.second.begin());
			Vector<Rational> base_vertex = separated_vertices.row(base_vertex_index);
			for(int l = 0; l < lineality.rows(); l++) {
				linealityValues[l] = ( evaluate_polynomial(num, base_vertex + lineality.row(l)) -
						evaluate_polynomial(den, base_vertex + lineality.row(l)) ) -
					vertexValues[base_vertex_index];
			}

			function.take("LINEALITY_VALUES") << linealityValues;
		}//END computeGeometricFunctionData

	/*
	 * @brief This takes two polynomials defining a rational function on an affine chart
	 * and homogenizes them to produce the (equivalent) rational function on homogeneous coordinates.
	 * @param Polynomial num The numerator
	 * @param Polynomial den The denominator
	 * @param int chart The index of the homogenizing variable
	 * @tparam Addition Min or Max
	 * @return RationalFunction
	 */
	template <typename Addition>
		perl::Object homogenize_quotient(Polynomial<TropicalNumber<Addition> > num, 
				Polynomial<TropicalNumber<Addition> > den,
				int chart) {
			Matrix<int> num_mons = num.monomials_as_matrix();
			Vector<TropicalNumber<Addition> > num_coefs = num.coefficients_as_vector();
			Matrix<int> den_mons = den.monomials_as_matrix();
			Vector<TropicalNumber<Addition> > den_coefs = den.coefficients_as_vector();

			if(num_mons.cols() != den_mons.cols()) 
				throw std::runtime_error("Cannot homogenize quotient. Number of variables is different.");

			if(chart <0 || chart > num_mons.cols())
				throw std::runtime_error("Illegal chart index.");

			//Compute missing degrees
			int total_degree = std::max(polynomial_degree(num), polynomial_degree(den));
			Vector<int> num_missing = total_degree*ones_vector<int>(num_mons.rows()) -
				degree_vector(num);
			Vector<int> den_missing = total_degree*ones_vector<int>(den_mons.rows()) -
				degree_vector(den);

			//Insert at right position
			Matrix<int> new_num_mons(num_mons.rows(),num_mons.cols()+1);
			new_num_mons.col(chart) = num_missing;
			new_num_mons.minor(All,~scalar2set(chart)) = num_mons;
			Matrix<int> new_den_mons(den_mons.rows(), den_mons.cols()+1);
			new_den_mons.col(chart) = den_missing;
			new_den_mons.minor(All,~scalar2set(chart)) = den_mons;

			//Make ring and return result
			Ring<TropicalNumber<Addition> > r(num_mons.cols()+1);
			Polynomial<TropicalNumber<Addition> > new_num(new_num_mons, num_coefs,r);
			Polynomial<TropicalNumber<Addition> > new_den(new_den_mons, den_coefs,r);

			perl::Object result(perl::ObjectType::construct<Addition>("RationalFunction"));
			result.take("NUMERATOR") << new_num;
			result.take("DENOMINATOR") << new_den;
			return result;

		}//END homogenize_quotient

	/*
	 * @brief Takes two rational functions (which are not given as polynomial quotients) and
	 * computes the sum (classical, not tropical)
	 * @param RationalFunction f
	 * @param RationalFunction g
	 * @tparam Addition Min or Max
	 * @return RationalFunction
	 */
	template <typename Addition> 
		perl::Object add_rational_functions(perl::Object f, perl::Object g) {
			perl::Object fDomain = f.give("DOMAIN");
			perl::Object gDomain = g.give("DOMAIN");


			//Then compute the common refinement of the domains
			RefinementResult r = refinement(fDomain,gDomain,true,true,false,true);
			perl::Object nDomain = r.complex;
			Matrix<Rational> x_rayrep = r.rayRepFromX;
			Matrix<Rational> y_rayrep = r.rayRepFromY;
			Matrix<Rational> x_linrep = r.linRepFromX;
			Matrix<Rational> y_linrep = r.linRepFromY;

			Vector<Rational> f_rayval = f.give("VERTEX_VALUES");
			Vector<Rational> g_rayval = g.give("VERTEX_VALUES");
			Vector<Rational> f_linval = f.give("LINEALITY_VALUES");
			Vector<Rational> g_linval = g.give("LINEALITY_VALUES");

			Vector<Rational> fval = f_rayval | f_linval;
			Vector<Rational> gval = g_rayval | g_linval;

			Matrix<Rational> rays = nDomain.give("SEPARATED_VERTICES");
			Matrix<Rational> linspace = nDomain.give("LINEALITY_SPACE");

			//Now compute ray values
			Vector<Rational> rValues;
			for(int r = 0; r < rays.rows(); r++) {
				rValues |= (x_rayrep.row(r) * fval) + (y_rayrep.row(r) * gval);
			}
			//Now compute lin values
			Vector<Rational> lValues;
			for(int l = 0; l < linspace.rows(); l++) {
				lValues |= (x_linrep.row(l) * f_linval) + (y_linrep.row(l) * g_linval);
			}

			//Return result
			perl::Object func(perl::ObjectType::construct<Addition>("RationalFunction"));
			func.take("DOMAIN") << nDomain;
			func.take("VERTEX_VALUES") << rValues;
			func.take("LINEALITY_VALUES") << lValues;

			return func;
		}//END add_rational_functions

	// PERL WRAPPER //////////////////////////

	FunctionTemplate4perl("computePolynomialDomain<Addition>(Polynomial<TropicalNumber<Addition> >)");
	FunctionTemplate4perl("computeDomain<Addition>(RationalFunction<Addition>) : void");
	FunctionTemplate4perl("computeGeometricFunctionData<Addition>(RationalFunction<Addition>) : void");
	FunctionTemplate4perl("homogenize_quotient<Addition>(Polynomial<TropicalNumber<Addition> >,Polynomial<TropicalNumber<Addition> >;$=0)");
	FunctionTemplate4perl("add_rational_functions<Addition>(RationalFunction<Addition>,RationalFunction<Addition>)");

}}
