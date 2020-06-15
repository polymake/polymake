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

	---
	Copyright (c) 2016-2020
	Ewgenij Gawrilow, Michael Joswig, and the polymake team
	Technische Universität Berlin, Germany
	https://polymake.org

	Functions for the basic ruleset of TropicalRationalFunction
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
#include "polymake/tropical/rational_function.h"

namespace polymake { namespace tropical {

using ValuePair = std::pair<Vector<Rational>, Vector<Rational>>;

/**
 * @brief Computes the property [[DOMAIN]] from [[NUMERATOR]] and [[DENOMINATOR]].
 */
template <typename Addition>
void computeDomain(BigObject function)
{
  Polynomial<TropicalNumber<Addition>> num = function.give("NUMERATOR");
  Polynomial<TropicalNumber<Addition>> den = function.give("DENOMINATOR");

  BigObject domain_num = computePolynomialDomain(num);
  BigObject domain_den = computePolynomialDomain(den);

  RefinementResult r = refinement(domain_num, domain_den, false,false,false,true,false);
  r.complex.give("PURE");
  function.take("DOMAIN") << r.complex;
}

/**
 * @brief Computes properties [[VERTEX_VALUES]] and [[LINEALITY_VALUES]]
 * from [[[DOMAIN]], [NUMERATOR]] and [[DENOMINATOR]].
 */
template <typename Addition>
void computeGeometricFunctionData(BigObject function)
{
  Polynomial<TropicalNumber<Addition>> num = function.give("NUMERATOR");
  Polynomial<TropicalNumber<Addition>> den = function.give("DENOMINATOR");
  BigObject domain = function.give("DOMAIN");

  // This just computes the associated vertices
  RefinementResult r = refinement(domain, domain, false,false,true,false,false);

  Matrix<Rational> separated_vertices = domain.give("SEPARATED_VERTICES");
  std::pair<Set<Int>, Set<Int>> vertex_list = far_and_nonfar_vertices(separated_vertices);
  separated_vertices = separated_vertices.minor(All, range_from(1));
  Matrix<Rational> lineality = domain.give("LINEALITY_SPACE");
  lineality = lineality.minor(All, range_from(1));
  Vector<Int> assocRep = r.associatedRep;

  Vector<Rational> vertexValues(separated_vertices.rows());
  Vector<Rational> linealityValues(lineality.rows());

  // Compute values for all nonfar vertices
  for (auto v = entire(vertex_list.second); !v.at_end(); ++v) {
    vertexValues[*v] = evaluate_polynomial(num, separated_vertices.row(*v)) -
      evaluate_polynomial(den, separated_vertices.row(*v));
  }

  // For all far vertices, compute slope with respect to associated vertex
  for (auto v = entire(vertex_list.first); !v.at_end(); ++v) {
    Vector<Rational> associated_vertex = separated_vertices.row(assocRep[*v]);
    vertexValues[*v] = ( evaluate_polynomial(num, associated_vertex + separated_vertices.row(*v)) -
                         evaluate_polynomial(den, associated_vertex + separated_vertices.row(*v))) -
      vertexValues[assocRep[*v]];
  }

  function.take("VERTEX_VALUES") << vertexValues;

  // Same for lineality space generators - we use a fixed vertex as base vertex
  Int base_vertex_index = *(vertex_list.second.begin());
  Vector<Rational> base_vertex = separated_vertices.row(base_vertex_index);
  for (Int l = 0; l < lineality.rows(); ++l) {
    linealityValues[l] = ( evaluate_polynomial(num, base_vertex + lineality.row(l)) -
                           evaluate_polynomial(den, base_vertex + lineality.row(l)) ) -
      vertexValues[base_vertex_index];
  }

  function.take("LINEALITY_VALUES") << linealityValues;
}

/*
 * @brief This takes two polynomials defining a rational function on an affine chart
 * and homogenizes them to produce the (equivalent) rational function on homogeneous coordinates.
 * @param Polynomial num The numerator
 * @param Polynomial den The denominator
 * @param Int chart The index of the homogenizing variable
 * @tparam Addition Min or Max
 * @return TropicalRationalFunction
 */
template <typename Addition>
BigObject homogenize_quotient(const Polynomial<TropicalNumber<Addition>>& num, 
                                 const Polynomial<TropicalNumber<Addition>>& den,
                                 Int chart)
{
  Matrix<Int> num_mons = num.monomials_as_matrix();
  Vector<TropicalNumber<Addition> > num_coefs = num.coefficients_as_vector();
  Matrix<Int> den_mons = den.monomials_as_matrix();
  Vector<TropicalNumber<Addition> > den_coefs = den.coefficients_as_vector();

  if (num_mons.cols() != den_mons.cols()) 
    throw std::runtime_error("Cannot homogenize quotient. Number of variables is different.");

  if (chart <0 || chart > num_mons.cols())
    throw std::runtime_error("Illegal chart index.");

  // Compute missing degrees
  Int total_degree = std::max(polynomial_degree(num), polynomial_degree(den));
  Vector<Int> num_missing = same_element_vector(total_degree, num_mons.rows()) - degree_vector(num);
  Vector<Int> den_missing = same_element_vector(total_degree, den_mons.rows()) - degree_vector(den);

  // Insert at right position
  Matrix<Int> new_num_mons(num_mons.rows(),num_mons.cols()+1);
  new_num_mons.col(chart) = num_missing;
  new_num_mons.minor(All,~scalar2set(chart)) = num_mons;
  Matrix<Int> new_den_mons(den_mons.rows(), den_mons.cols()+1);
  new_den_mons.col(chart) = den_missing;
  new_den_mons.minor(All,~scalar2set(chart)) = den_mons;

  // Make ring and return result
  Polynomial<TropicalNumber<Addition> > new_num(num_coefs, new_num_mons);
  Polynomial<TropicalNumber<Addition> > new_den(den_coefs, new_den_mons);

  return BigObject("TropicalRationalFunction", mlist<Addition>(),
                   "NUMERATOR", new_num,
                   "DENOMINATOR", new_den);
}

/*
 * @brief Takes two rational functions (which are not given as polynomial quotients) and
 * computes the sum (classical, not tropical)
 * @param TropicalRationalFunction f
 * @param TropicalRationalFunction g
 * @tparam Addition Min or Max
 * @return TropicalRationalFunction
 */
template <typename Addition> 
BigObject add_rational_functions(BigObject f, BigObject g)
{
  BigObject fDomain = f.give("DOMAIN");
  BigObject gDomain = g.give("DOMAIN");

  // Then compute the common refinement of the domains
  RefinementResult r = refinement(fDomain,gDomain,true,true,false,true);
  BigObject nDomain = r.complex;
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

  // Now compute ray values
  Vector<Rational> rValues;
  for (Int v = 0; v < rays.rows(); ++v) {
    rValues |= (x_rayrep.row(v) * fval) + (y_rayrep.row(v) * gval);
  }
  // Now compute lin values
  Vector<Rational> lValues;
  for (Int l = 0; l < linspace.rows(); ++l) {
    lValues |= (x_linrep.row(l) * f_linval) + (y_linrep.row(l) * g_linval);
  }

  // Return result
  return BigObject("TropicalRationalFunction", mlist<Addition>(),
                   "DOMAIN", nDomain,
                   "VERTEX_VALUES", rValues,
                   "LINEALITY_VALUES", lValues);
}

FunctionTemplate4perl("computePolynomialDomain<Addition>(Polynomial<TropicalNumber<Addition>>)");
FunctionTemplate4perl("computeDomain<Addition>(TropicalRationalFunction<Addition>)");
FunctionTemplate4perl("computeGeometricFunctionData<Addition>(TropicalRationalFunction<Addition>)");
FunctionTemplate4perl("homogenize_quotient<Addition>(Polynomial<TropicalNumber<Addition>>, Polynomial<TropicalNumber<Addition>>; $=0)");
FunctionTemplate4perl("add_rational_functions<Addition>(TropicalRationalFunction<Addition>, TropicalRationalFunction<Addition>)");

} }
