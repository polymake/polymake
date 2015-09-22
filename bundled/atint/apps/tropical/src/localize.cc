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

	Contains functions to localize a cycle at some cones.
	*/

#include "polymake/client.h"
#include "polymake/Matrix.h"
#include "polymake/Rational.h"
#include "polymake/Vector.h"
#include "polymake/tropical/refine.h"
#include "polymake/tropical/LoggingPrinter.h"
#include "polymake/tropical/misc_tools.h"
#include "polymake/tropical/separated_data.h"
#include "polymake/tropical/specialcycles.h"
#include "polymake/tropical/thomog.h"


namespace polymake { namespace tropical {
	using namespace atintlog::donotlog;
	//using namespace atintlog::dolog;
	//using namespace atintlog::dotrace;

	///////////////////////////////////////////////////////////////////////////////////////

	//Documentation see perl wrapper
	template <typename Addition>
		perl::Object local_restrict(perl::Object complex, IncidenceMatrix<> cones) {
			//Extract values
			IncidenceMatrix<> maximalCones = complex.give("MAXIMAL_POLYTOPES");
			Matrix<Rational> rays = complex.give("VERTICES");
			Matrix<Rational> linspace = complex.give("LINEALITY_SPACE");
			Vector<Integer> weights = complex.give("WEIGHTS");

			//Find out which cones are no longe compatible
			Set<int> remainingCones;
			for(int c = 0; c < maximalCones.rows(); c++) {
				if(is_coneset_compatible(maximalCones.row(c), cones)) {
					remainingCones += c;
				}
			}

			//Adapt cone description and ray indices
			maximalCones = maximalCones.minor(remainingCones,All);
			weights = weights.slice(remainingCones);
			Set<int> usedRays = accumulate(rows(maximalCones),operations::add());
			
			//We have to take care when adapting the local restriction:
			// If cones is input by hand, it may have less columns then there are rays left.
			IncidenceMatrix<> newlocalcones(cones.rows(), rays.rows());
				newlocalcones.minor(All, sequence(0, cones.cols())) = cones;
			newlocalcones = newlocalcones.minor(All,usedRays);

			rays = rays.minor(usedRays,All);
			IncidenceMatrix<> newMaximalCones(maximalCones);
			newMaximalCones = newMaximalCones.minor(All,usedRays);

						perl::Object result(perl::ObjectType::construct<Addition>("Cycle"));
			result.take("PROJECTIVE_VERTICES") << rays;
			result.take("MAXIMAL_POLYTOPES") << newMaximalCones;
			result.take("LINEALITY_SPACE") << linspace;
			result.take("WEIGHTS") << weights;
			result.take("LOCAL_RESTRICTION") << newlocalcones;

			return result;
		}

	///////////////////////////////////////////////////////////////////////////////////////

	//Documentation see perl wrapper
	template <typename Addition>
		perl::Object local_vertex(perl::Object complex, int vertex) {
			//Convert vertex to incidence matrix
			Vector<Set<int> > matrix;
			Set<int> set;
			set += vertex;
			matrix |= set;
			return local_restrict<Addition>(complex, IncidenceMatrix<>(matrix));
		}

	///////////////////////////////////////////////////////////////////////////////////////

	//Documentation see perl wrapper
	template <typename Addition>
		perl::Object local_codim_one(perl::Object complex, int face) {
			//Convert codim face index to incidence matrix
			IncidenceMatrix<> codim = complex.give("CODIMENSION_ONE_POLYTOPES");
			if(face >= codim.rows()) {
				throw std::runtime_error("Cannot localize at codim one face: Index is out of bounds.");
			}
			Vector<Set<int> > matrix;
			matrix |= codim.row(face);
			return local_restrict<Addition>(complex, IncidenceMatrix<>(matrix));
		}

	///////////////////////////////////////////////////////////////////////////////////////

	//Documentation see perl wrapper
	template <typename Addition>
		perl::Object local_point(perl::Object complex, Vector<Rational> point) {
			//Normalize the vertex
			if(point.dim() <= 1) {
				throw std::runtime_error("Cannot localize at point: Point dimension is too low");
			}
			if(point[0] == 0) {
				throw std::runtime_error("Cannot localize at point: Point is not a vertex (or not given with leading coordinate");
			}
			point /= point[0];

			//First we refine the complex
			RefinementResult r = refinement(complex, orthant_subdivision<Addition>(point),false,false,false,true);
			perl::Object refinedComplex = r.complex;

			//Then we look for the vertex
			Matrix<Rational> rays = refinedComplex.give("VERTICES");
			Set<int> vertices = far_and_nonfar_vertices(rays).second; 
			int pointindex = -1;
			for(Entire<Set<int> >::iterator v = entire(vertices); !v.at_end(); v++) {
				if(tdehomog_vec(Vector<Rational>(rays.row(*v))) == tdehomog_vec(point)) {
					pointindex = *v; break;
				}
			}

			//If we didn't find it, throw an error
			if(pointindex == -1) throw std::runtime_error("Cannot localize at point: Is not contained in support of complex.");

			//Otherwise localize
			return local_vertex<Addition>(refinedComplex,pointindex);

		}


	// ------------------------- PERL WRAPPERS ---------------------------------------------------

	UserFunctionTemplate4perl("# @category Local computations"
			"# This takes a tropical variety and an IncidenceMatrix describing a set"
			"# of cones (not necessarily maximal ones) of this variety. It will then"
			"# create a variety that contains all compatible maximal cones and is"
			"# locally restricted to the given cone set."
			"# @param Cycle<Addition> complex An arbitrary weighted complex"
			"# @param IncidenceMatrix cones A set of cones, indices refer to VERTICES"
			"# @return Cycle<Addition> The same complex, locally restricted to the given"
			"# cones",
			"local_restrict<Addition>(Cycle<Addition>,$)");

	UserFunctionTemplate4perl("#@category Local computations"
			"# This takes a weighted complex and an index of one of its vertices "
			"# (the index is to be understood in VERTICES)"
			"# It then localizes the variety at this vertex. The index should never"
			"# correspond to a far vertex in a complex, since this would not be a cone"
			"# @param Cycle<Addition> complex An arbitrary weighted complex"
			"# @param Int ray The index of a ray/vertex in RAYS"
			"# @return Cycle<Addition> The complex locally restricted to the given vertex",
			"local_vertex<Addition>(Cycle<Addition>,$)");

	UserFunctionTemplate4perl("# @category Local computations"
			"# This takes a weighted complex and an index of one of its codimension one faces"
			"# (The index is in CODIMENSION_ONE_POLYTOPES) and computes the complex locally restricted"
			"# to that face"
			"# @param Cycle<Addition> complex An arbitrary weighted complex"
			"# @param Int face An index of a face in CODIMENSION_ONE_POLYTOPES"
			"# @return Cycle<Addition> The complex locally restricted to the given face",
			"local_codim_one<Addition>(Cycle<Addition>,$)");

	UserFunctionTemplate4perl("# @category Local computations"
			"# This takes a weighted complex and an arbitrary vertex in homogeneous "
			"# coordinates (including the leading coordinate) that is supposed to lie "
			"# in the support of the complex."
			"# It then refines the complex such that the vertex is a cell in the polyhedral "
			"# structure and returns the complex localized at this vertex"
			"# @param Cycle<Addition> complex An arbitrary weighted complex"
			"# @param Vector<Rational> v A vertex in homogeneous coordinates and with leading coordinate. It should lie"
			"# in the support of the complex (otherwise an error is thrown)"
			"# @return Cycle<Addition> The complex localized at the vertex",
			"local_point<Addition>(Cycle<Addition>,$)");

}}

