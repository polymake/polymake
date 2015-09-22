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
#include "polymake/tropical/LoggingPrinter.h"

namespace polymake { namespace tropical {

	using namespace atintlog::donotlog;
	//   using namespace atintlog::dolog;
	//   using namespace atintlog::dotrace;

	///////////////////////////////////////////////////////////////////////////////////////

	//Documentation see perl wrapper
	template <typename Addition>
	perl::Object hurwitz_marked_cycle(int k, Vector<int> degree, 
			Vector<Rational> pullback_points = Vector<Rational>()) {

		//First, compute the psi-class product
		int n = degree.dim();

		//Sanity check
		if(k < 0 || k > n) {
			throw std::runtime_error("Marked Hurwitz cycle: Invalid dimension parameter.");
		}
		
		if(pullback_points.dim() < n-3-k) {
			pullback_points |= zero_vector<Rational>(n-3-k - pullback_points.dim());
		}

		int big_n = 2*n - k- 2;
		Vector<int> exponents = zero_vector<int>(n) | ones_vector<int>(n-2-k);
		//dbgtrace << "Computing psi product in M_N, N = " << big_moduli_dim << " with exponents " << exponents << endl;
		perl::Object P = psi_product<Addition>(big_n,exponents);

		if(n == 4) return P;

		//Compute evalutation maps and pullbacks
		perl::ArgList pb_functions;
		Matrix<Rational> rat_degree(degree.dim(),0);
		Vector<Rational> zero_translate(2);
		rat_degree |= degree;
		for(int i = n+2; i <= 2*n-2-k; i++) {
			//dbgtrace << "Computing evaluation map pull back for i = " << i-n-1 << endl;
			perl::Object evi = evaluation_map<Addition>(n-2-k, thomog(rat_degree,0,false), i-n-1);
			Matrix<Rational> evi_matrix = evi.give("MATRIX");
				evi_matrix = tdehomog_morphism(evi_matrix, zero_translate).first;
			//Pulling back p_i = max(x,p_i) * R means we take the vector representing the morphism and 
			//attach a row below that has p_i at the end
			evi_matrix /= zero_vector<Rational>(evi_matrix.cols());
			//Since we restrict ourselves to M_0,N x {0}, we actually ignore the last coefficient
			//of ev_i and replace it by the constant coefficient 0 (for the min-max-function)
			Matrix<int > monoms(evi_matrix.minor(All,~scalar2set(evi_matrix.cols()-1)));
			Vector<TropicalNumber<Addition> > coeffs(2);
				coeffs[0] = TropicalNumber<Addition>(0);
				coeffs[1] = TropicalNumber<Addition>(pullback_points[i-n-2]);
			//dbgtrace << "Pullback evaluation matrix is " << evi_matrix << endl;
			Ring<TropicalNumber<Addition> > tropring(evi_matrix.cols()-1);
			Polynomial<TropicalNumber<Addition> > p(monoms,coeffs,tropring);
			perl::Object pb = CallPolymakeFunction("rational_fct_from_affine_numerator",p);
			pb_functions << pb;
		}//END compute pullback functions


		//Now compute the divisor
		P = CallPolymakeFunction("divisor",P, pb_functions); 

		return P;

	}//END function hurwitz_pre_cycle

	UserFunctionTemplate4perl("# @category Hurwitz cycles"
			"# Computes the marked k-dimensional tropical Hurwitz cycle H_k(degree)"
			"# @param int k The dimension of the Hurwitz cycle"
			"# @param Vector<Int> degree The degree of the covering. The sum over all entries should "
			"# be 0 and if n := degree.dim, then 0 <= k <= n-3"
			"# @param Vector<Rational> pullback_points The points p_i that should be pulled back to "
			"# determine the Hurwitz cycle (in addition to 0). Should have length n-3-k. If it is not given, "
			"# all p_i are by default equal to 0 (same for missing points)"
			"# @tparam Addition Min or Max"
			"# @return Cycle<Addition> The marked Hurwitz cycle H~_k(degree)",
			"hurwitz_marked_cycle<Addition>($, Vector<Int>; Vector<Rational> = new Vector<Rational>())");

}}
