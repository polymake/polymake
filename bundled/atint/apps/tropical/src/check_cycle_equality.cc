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

	Contains a function to check whether two cycles are identical.
	*/


#include "polymake/client.h"
#include "polymake/Matrix.h"
#include "polymake/Rational.h"
#include "polymake/Vector.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/linalg.h"
#include "polymake/tropical/LoggingPrinter.h"
#include "polymake/tropical/thomog.h"


namespace polymake { namespace tropical {


	//Documentation see perl wrapper
	template <typename Addition>
		bool check_cycle_equality(perl::Object X, perl::Object Y, bool check_weights = true) {

			//dbgtrace << "Extracting values " << endl;
			//Extract values
			Matrix<Rational> xrays = X.give("VERTICES");
			xrays = tdehomog(xrays);
			IncidenceMatrix<> xcones = X.give("MAXIMAL_POLYTOPES");
			Matrix<Rational> xlin = X.give("LINEALITY_SPACE");
			xlin = tdehomog(xlin);
			int xambi = X.give("PROJECTIVE_AMBIENT_DIM");
			Vector<Integer> xweights;
			if(X.exists("WEIGHTS")) {
				X.give("WEIGHTS") >> xweights;
			}
			else check_weights = false;

			Matrix<Rational> yrays = Y.give("VERTICES");
			yrays = tdehomog(yrays);
			IncidenceMatrix<> ycones = Y.give("MAXIMAL_POLYTOPES");
			Matrix<Rational> ylin = Y.give("LINEALITY_SPACE");
			ylin = tdehomog(ylin);
			int yambi = Y.give("PROJECTIVE_AMBIENT_DIM");
			Vector<Integer> yweights;
			if(Y.exists("WEIGHTS")) {
				Y.give("WEIGHTS") >> yweights;
			}
			else check_weights = false;

			//dbgtrace << "Checking equality of dimensions " << endl;

			//Check dimensional equality
			if(xambi != yambi) return false;

			//dbgtrace << "Checkking equality of lineality spaces " << endl;

			//Check equality of lineality spaces
			if(rank(xlin) == rank(ylin)) {
				if(rank(xlin / ylin) > rank(xlin)) return false;
			}
			else return false;

			//dbgtrace << "Finding ray permutation " << endl;

			//Find ray permutation
			if(xrays.rows() != yrays.rows()) return false;
			Map<int,int> permutation;
			for(int x = 0; x < xrays.rows(); x++) {
				for(int y = 0; y < yrays.rows(); y++) {
					//Check if yray = xray modulo lineality
					Matrix<Rational> diff_rays = xrays.minor(scalar2set(x),All) - yrays.minor(scalar2set(y),All);
					if(rank(xlin) == rank(xlin / diff_rays)) {
						permutation[x] = y;
						break;
					}
					//If we arrive here, there is no ray matching x
					if(y == yrays.rows()-1) return false;
				}
			}//END compute ray permutation

			//dbgtrace << "Ray permutation is " << permutation << endl;

			//dbgtrace << "Matching cones " << endl;

			//Now check if all cones are equal
			Set<int> matched_cones;
			for(int xc = 0; xc < xcones.rows(); xc++) {
				//dbgtrace << "Matching cone " << xcones.row(xc) << endl;
				//Compute permuted cone
				Set<int> perm_cone = attach_operation(xcones.row(xc), pm::operations::associative_access<Map<int,int>, int>(&permutation));
				//dbgtrace << "Permuted cone is " << perm_cone << endl;
				//Find this cone in Y
				for(int yc = 0; yc < ycones.rows(); yc++) {
					if(!matched_cones.contains(yc)) {
						if(ycones.row(yc).size() == perm_cone.size()) {
							//dbgtrace << "Comparing with " << ycones.row(yc) << endl;
							if((ycones.row(yc) * perm_cone).size() == perm_cone.size()) {
								matched_cones += yc;
								//Check equality of weights, if necessary
								if(check_weights) {
									//dbgtrace << "Checking weight equality" << endl;
									if(xweights[xc] != yweights[yc]) return false;
								}
								break;
							}
						}//END check cone equality
					}
					//If we arrive here, there is no match:
					if(yc == ycones.rows()-1) return false;

				}//END iterate Y cones
			}//END iterate X cones

			//dbgtrace << "Checking if all cones were matched " << endl;

			//Check if we actually matched ALL Y cones
			if(matched_cones.size() < ycones.rows()) return false;

			return true;

		}//END function check_cycle_equality

	UserFunctionTemplate4perl("# @category Basic polyhedral operations"
			"# This takes two pure-dimensional polyhedral complexes and checks if they are equal"
			"# i.e. if they have the same lineality space, the same rays (modulo lineality space)"
			"# and the same cones. Optionally, it can also check if the weights are equal"
			"# @param Cycle<Addition> X A weighted complex"
			"# @param Cycle<Addition> Y A weighted complex"
			"# @param Bool check_weights Whether the algorithm should check for equality of weights. "
			"# This parameter is optional and true by default"
			"# @return Bool Whether the cycles are equal",
			"check_cycle_equality<Addition>(Cycle<Addition>,Cycle<Addition>;$=1)");
}}
