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

	Implements the computation of a recession fan.
	*/


#include "polymake/client.h"
#include "polymake/Rational.h"
#include "polymake/Set.h"
#include "polymake/Matrix.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Vector.h"
#include "polymake/tropical/misc_tools.h"
#include "polymake/tropical/make_complex.h"
#include "polymake/linalg.h"

namespace polymake { namespace tropical {

	//Documentation see perl wrapper
	template <typename Addition>
	perl::Object recession_fan(perl::Object complex) {

		//Special cases
		if (call_function("is_empty", complex))
			return complex;

		//Extract values
		Matrix<Rational> rays = complex.give("VERTICES");
		IncidenceMatrix<> cones = complex.give("MAXIMAL_POLYTOPES");
		Matrix<Rational> linspace = complex.give("LINEALITY_SPACE");
		Vector<Integer> weights = complex.give("WEIGHTS");
		int dim = complex.give("PROJECTIVE_DIM");
		std::string desc = complex.description();

		std::pair<Set<int>, Set<int> > sorted_vertices = far_and_nonfar_vertices(rays);
		Set<int> directional = sorted_vertices.first;

		//If there is only one vertex, shift it.
		if(directional.size() == rays.rows()-1) {
			int vertex = *(sorted_vertices.second.begin());
			rays.row(vertex) = unit_vector<Rational>(rays.cols(),0);
			perl::Object result(perl::ObjectType::construct<Addition>("Cycle"));
					result.take("PROJECTIVE_VERTICES") << rays;
					result.take("MAXIMAL_POLYTOPES") << cones;
					result.take("WEIGHTS") << weights;
			return result;
		}

		Matrix<Rational> newrays = rays.minor(directional,All);
		Matrix<Rational> newlineality = linspace;
		Vector<Integer> newweights;

		//Re-map ray indices
		Map<int,int> indexMap; int i = 0;
		for (auto d = entire(directional); !d.at_end(); ++d, ++i) {
                  indexMap[*d] = i;
		}

		//We compute the recession cone of each cell
		Vector<Set<int> > rec_cones;
		for(int mc = 0; mc < cones.rows(); mc++) {
			Set<int> mcDirectional = directional * cones.row(mc);
			//Compute that it has the right dimension
			int mcDim = rank(rays.minor(mcDirectional,All));
			if (mcDirectional.size() > 0 && mcDim == dim) {
                          Set<int> transformCone{ indexMap.map(mcDirectional) };
                          rec_cones |= transformCone;
                          newweights |= weights[mc];
			}
		}

		//Add vertex
		newrays /= unit_vector<Rational>(newrays.cols(),0);
		for(int mc = 0; mc < rec_cones.dim(); mc++) {
			rec_cones[mc] += scalar2set(newrays.rows()-1);
		}

		std::ostringstream newdesc;
		newdesc << "Recession fan of \"" << desc << "\"";

		//Compute the complexification of the recession cones
		perl::Object cplxify = make_complex<Addition>(newrays,rec_cones,newweights);
		//Extract its values and put them into the result
		perl::Object result(perl::ObjectType::construct<Addition>("Cycle"));
		result.take("VERTICES") << cplxify.give("VERTICES");
		result.take("MAXIMAL_POLYTOPES") << cplxify.give("MAXIMAL_POLYTOPES");
		result.take("WEIGHTS") << cplxify.give("WEIGHTS");
		result.take("LINEALITY_SPACE") << newlineality;
		result.set_description() << newdesc.str();
		return result;
	}



	UserFunctionTemplate4perl("# @category Basic polyhedral operations"
    			"# Computes the recession fan of a tropical variety. WARNING: This is a highly experimental"
		      "# function. If it works at all, it is likely to take a very long time for larger objects."
		      "# @param Cycle complex A tropical variety"
		      "# @return Cycle A tropical fan, the recession fan of the complex",
		      "recession_fan<Addition>(Cycle<Addition>)");


}}

