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
	Copyright (c) 2016-2021
	Ewgenij Gawrilow, Michael Joswig, and the polymake team
	Technische Universität Berlin, Germany
	https://polymake.org

	Computes marked Hurwitz cycles
	*/

#include "polymake/client.h"
#include "polymake/Matrix.h"
#include "polymake/Rational.h"
#include "polymake/Vector.h"
#include "polymake/TropicalNumber.h"
#include "polymake/Polynomial.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/tropical/thomog.h"
#include "polymake/tropical/psi_classes.h"
#include "polymake/tropical/morphism_special.h"
#include "polymake/tropical/morphism_thomog.h"

namespace polymake { namespace tropical {


///////////////////////////////////////////////////////////////////////////////////////

//Documentation see perl wrapper
template <typename Addition>
BigObject hurwitz_marked_cycle(Int k, Vector<Int> degree,
                                  Vector<Rational> pullback_points = Vector<Rational>())
{
  //First, compute the psi-class product
  Int n = degree.dim();

  //Sanity check
  if (k < 0 || k > n) {
    throw std::runtime_error("Marked Hurwitz cycle: Invalid dimension parameter.");
  }

  if (pullback_points.dim() < n-3-k) {
    pullback_points |= zero_vector<Rational>(n-3-k - pullback_points.dim());
  }

  const Int big_n = 2*n - k - 2;
  Vector<Int> exponents = zero_vector<Int>(n) | ones_vector<Int>(n-2-k);
  BigObject P = psi_product<Addition>(big_n, exponents);

  if (n == 4) return P;

  //Compute evalutation maps and pullbacks
  std::vector<BigObject> pb_functions;
  pb_functions.reserve(big_n-n-1);
  Matrix<Rational> rat_degree(degree.dim(),0);
  Vector<Rational> zero_translate(2);
  rat_degree |= degree;
  for (Int i = n+2; i <= big_n; ++i) {
    BigObject evi = evaluation_map<Addition>(n-2-k, thomog(rat_degree,0,false), i-n-1);
    Matrix<Rational> evi_matrix = evi.give("MATRIX");
    evi_matrix = tdehomog_morphism(evi_matrix, zero_translate).first;

    //Pulling back p_i = max(x,p_i) * R means we take the vector representing the morphism and
    //attach a row below that has p_i at the end
    evi_matrix /= zero_vector<Rational>(evi_matrix.cols());

    //Since we restrict ourselves to M_0,N x {0}, we actually ignore the last coefficient
    //of ev_i and replace it by the constant coefficient 0 (for the min-max-function)
    Matrix<Int> monoms(evi_matrix.minor(All, sequence(0, evi_matrix.cols()-1)));

    Vector<TropicalNumber<Addition> > coeffs(2);
    coeffs[0] = TropicalNumber<Addition>(0);
    coeffs[1] = TropicalNumber<Addition>(pullback_points[i-n-2]);

    Polynomial<TropicalNumber<Addition>> p(coeffs, monoms);
    BigObject pb = call_function("rational_fct_from_affine_numerator", p);
    pb_functions.push_back(pb);
  }//END compute pullback functions

  //Now compute the divisor
  P = call_function("divisor", P, perl::unroll(pb_functions));

  return P;

}//END function hurwitz_pre_cycle

UserFunctionTemplate4perl("# @category Hurwitz cycles"
                          "# Computes the marked k-dimensional tropical Hurwitz cycle H_k(degree)"
                          "# @param Int k The dimension of the Hurwitz cycle"
                          "# @param Vector<Int> degree The degree of the covering. The sum over all entries should "
                          "# be 0 and if n := degree.dim, then 0 <= k <= n-3"
                          "# @param Vector<Rational> pullback_points The points p_i that should be pulled back to "
                          "# determine the Hurwitz cycle (in addition to 0). Should have length n-3-k. If it is not given, "
                          "# all p_i are by default equal to 0 (same for missing points)"
                          "# @tparam Addition Min or Max"
                          "# @return Cycle<Addition> The marked Hurwitz cycle H~_k(degree)",
                          "hurwitz_marked_cycle<Addition>($, Vector<Int>; Vector<Rational> = new Vector<Rational>())");
} }
