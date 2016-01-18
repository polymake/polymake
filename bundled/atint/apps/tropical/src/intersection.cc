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

	Implements the description of the intersection product in the projective
	torus given by Jensen and Yu.
	*/

#include "polymake/client.h"
#include "polymake/Matrix.h"
#include "polymake/Rational.h"
#include "polymake/Vector.h"
#include "polymake/PowerSet.h"
#include "polymake/Array.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/linalg.h"
#include "polymake/Smith_normal_form.h"
#include "polymake/RandomGenerators.h"
#include "polymake/tropical/thomog.h"
#include "polymake/tropical/lattice.h"
#include "polymake/tropical/specialcycles.h"
#include "polymake/tropical/solver_def.h"
#include "polymake/tropical/codim_one_with_locality.h"
#include "polymake/tropical/convex_hull_tools.h"
#include "polymake/tropical/LoggingPrinter.h"

namespace polymake { namespace tropical { 


    using namespace atintlog::donotlog;
    //using namespace atintlog::dolog;
//     using namespace atintlog::dotrace;

	//Documentation see perl wrapper
	Integer lattice_index(const Matrix<Integer> &lattice_rays) {
		//Compute the Smith Normal form 
		SmithNormalForm<Integer> solution = smith_normal_form(lattice_rays);

		Integer result = 1;
		for(int i = 0; i < solution.rank; i++) {
			result *= solution.form(i,i);
		}

		return abs(result);

	}

	///////////////////////////////////////////////////////////////////////////////////////

	/**
	  @brief Computes the Star around a point in a given set of cones
	  @param Vector<Rational> point The point around which the star is to be computed, given in non-trop.-homog. coordinates
	  @param Matrix<Rational> rays The rays of the surrounding cones (needn't contain point) in non-trop.-homog. coordinates
	  @param Vector<Set<int> > cones The surrounding cones (i.e. should all contain point). Should form a complex.
	  @param Matrix<Rational> result_rays Will contain the rays of the result (without leading zeros). Might not be irredundant
	  @param Vector<Set<int> > result_cones Will contain the cones of the result. A cone might be given by a redundant list of rays. The cones will be in the exact same order as in "cones", i.e. the i-th fan cone is the star of the i-th cone at point.
	  */
	void computeStar(const Vector<Rational> &point, const Matrix<Rational> &rays, const IncidenceMatrix<> &cones,
			Matrix<Rational> &result_rays, Vector<Set<int> > &result_cones) {
		//Prepare result variables
		result_rays.resize(0,rays.cols()-1);


		Matrix<Rational> fan_rays = rays.minor(All,~scalar2set(0));
		Vector<Rational> fan_point = point.slice(~scalar2set(0));
		//Iterate all surrounding cones
		for(int sc = 0; sc < cones.rows(); sc++) {
			Set<int> cone;
			Set<int> surroundCone = cones.row(sc);
			for(Entire<Set<int> >::iterator r = entire(surroundCone); !r.at_end(); r++) {
				if(rays(*r,0) == 0) {
					result_rays /= fan_rays.row(*r);
				}
				else {
					result_rays /= (fan_rays.row(*r) - fan_point);
				}
				cone += (result_rays.rows()-1);
			}
			result_cones |= cone;
		}

		cdd_normalize_rays(result_rays,false);
	}

	///////////////////////////////////////////////////////////////////////////////////////

	/**
	  @brief Computes the Minkowski multiplicity of two fans, i.e. find two cones that add up to full dimension. Then it chooses an interior vector in the difference and finds all cone differences containing it as an interior vector. For all these differences it adds the product of the weights times the lattice index of the sum of the lattices
	  */
	Integer computeFanMultiplicity(const Matrix<Rational> &xrays, const Matrix<Rational> &xlin, 
			const Vector<Set<int> > &xcones, const Vector<Integer> &xweights, const int xdim, 
			const Matrix<Rational> &yrays, const Matrix<Rational> &ylin, 
			const Vector<Set<int> > &ycones, const Vector<Integer> &yweights, const int ydim) {
		Integer weight(0);
		solver<Rational> sv;

		// First, we compute all H-representations of xcone - ycone, keeping
		// only full-dimensional ones
		Vector< Matrix<Rational> > full_dimensional_cones;
		Vector<int> full_dimensional_xindex; //Keep track of associated x- and ycones
		Vector<int> full_dimensional_yindex;
		for(int xc = 0; xc < xcones.dim(); xc++) {
			for(int yc = 0; yc < ycones.dim(); yc++) {
				//dbgtrace << "Having cones " << xrays.minor(xcones[xc],All) << ", \n" << yrays.minor(ycones[yc],All) << endl;
				Matrix<Rational> x_sub_rays = xrays.minor(xcones[xc],All);
				Matrix<Rational> y_sub_rays = (- yrays.minor(ycones[yc],All));
				std::pair<Matrix<Rational>, Matrix<Rational> > eqs = 
					sv.enumerate_facets(zero_vector<Rational>() | x_sub_rays / y_sub_rays, 
							zero_vector<Rational>() | (xlin / ylin),true,false);
				if(eqs.second.rows() == 0){
					//dbgtrace << "Is fulldimensional" << endl;
					full_dimensional_cones |= eqs.first;
					full_dimensional_xindex |= xc;
					full_dimensional_yindex |= yc;
				}
			}
		}

		//If there are no full-dimensional cones, the result is 0
		if(full_dimensional_cones.dim() == 0) return weight;

		//Otherwise, we need to compute a generic vector. We compute a 
		// random vector and go through all cones. We add up appropriate
		// weights for those cones containing the point in their interior.
		// If we find one that contains the point in its boundary, we 
		// create another interior point and try again.
		bool point_found;
		UniformlyRandom<Rational> random_gen;
		Vector<Rational> interior_point(xrays.cols()+1);
		//dbgtrace << "Generating generic point" << endl;
		do {
			weight = Integer(0);
			copy(random_gen.begin(), entire(interior_point));
			interior_point[0] = 1;
			point_found = true;
			//dbgtrace << "Trying " << interior_point << endl;
			//Now go through all full-dimensional cones
			for(int fullcone = 0; fullcone < full_dimensional_cones.dim(); fullcone++) {
				//If the cone is the full space, i.e. has no facets, we don't need to check containment
				bool is_interior = true;
				bool is_in_boundary = false;
				if(full_dimensional_cones[fullcone].rows() > 0) {
					//dbgtrace << "Checking fulldimension cone " << fullcone << endl;
					//dbgtrace << "Has facets " << full_dimensional_cones[fullcone] << endl;
					Vector<Rational> eq_check = full_dimensional_cones[fullcone] * interior_point;
					for(int c = 0; c < eq_check.dim(); c++) {
						if(eq_check[c] == 0) {
							is_in_boundary = true; break;
						}
						if(eq_check[c] < 0) {
							is_interior = false; break;
						}
					}//END check for interiorness
					// If its in the boundary of something, try another point.
					if(is_in_boundary) {
						//dbgtrace << "It is a boundary point. Trying another one..." << endl;
						point_found = false; break;
					}
				}
				//If its interior, add the appropriate weight.
				if(is_interior) {
					//dbgtrace << "Is interior point of this cone, computing weight..." << endl;
					//dbgtrace << "xweight: " << xweights[full_dimensional_xindex[fullcone]] << endl;
					//dbgtrace << "yweight: " << yweights[full_dimensional_yindex[fullcone]] << endl;
					Integer latticeIndex = lattice_index(
							lattice_basis_of_cone(
								xrays.minor(xcones[full_dimensional_xindex[fullcone]],All),xlin,xdim,false) / 
							lattice_basis_of_cone(
								yrays.minor(ycones[full_dimensional_yindex[fullcone]],All),ylin,ydim,false));
					//dbgtrace << "lattice: " << latticeIndex<< endl;
					weight += (xweights[full_dimensional_xindex[fullcone]] * yweights[full_dimensional_yindex[fullcone]] * latticeIndex);
				}

			}//END iterate full-dimensional cones
		} while(!point_found);


		return weight;
	}

	///////////////////////////////////////////////////////////////////////////////////////

	//Documentation see perl wrapper
	template <typename Addition>
	perl::ListReturn intersect_check_transversality(perl::Object X, perl::Object Y, bool ensure_transversality = false) {
		//Extract values
		int Xcodim = X.give("PROJECTIVE_CODIMENSION");
		int Xdim   = X.give("PROJECTIVE_DIM");
		int Ycodim = Y.give("PROJECTIVE_CODIMENSION");
		int Ydim   = Y.give("PROJECTIVE_DIM");
		int Xambi  = X.give("PROJECTIVE_AMBIENT_DIM");

		//dbgtrace << "Checking codimension" << endl;

		//If the codimensions of the varieties add up to something larger then CMPLX_AMBIENT_DIM, return the 0-cycle 
		if(Xcodim + Ycodim > Xambi) {
			perl::ListReturn zeroResult;
				zeroResult << empty_cycle<Addition>(Xambi);
				zeroResult << false;
				return zeroResult;
		}

		//dbgtrace << "Homogenizing where necessary" << endl;

		//Extract values
		Matrix<Rational> xrays = X.give("VERTICES");
			xrays = tdehomog(xrays);
		Matrix<Rational> xlin = X.give("LINEALITY_SPACE");
			xlin = tdehomog(xlin);
		IncidenceMatrix<> xcones = X.give("MAXIMAL_POLYTOPES");
		Vector<Integer> xweights = X.give("WEIGHTS");
		int xambdim = X.give("PROJECTIVE_AMBIENT_DIM");

		Matrix<Rational> yrays = Y.give("VERTICES");
			yrays = tdehomog(yrays);
		Matrix<Rational> ylin = Y.give("LINEALITY_SPACE");
			ylin = tdehomog(ylin);
		IncidenceMatrix<> ycones = Y.give("MAXIMAL_POLYTOPES");
		Vector<Integer> yweights = Y.give("WEIGHTS");
		int yambdim = Y.give("PROJECTIVE_AMBIENT_DIM");

		if(xambdim != yambdim) {
			throw std::runtime_error("Cannot compute intersection product: Cycles live in different spaces.");
		}

		//Compute the expected dimension of the intersection product 
		int k = Xambi - (Xcodim + Ycodim);

		//Compute the intersection complex
		fan_intersection_result f = cdd_fan_intersection(xrays,xlin,xcones,yrays,ylin,ycones);

		//Now we compute the k-skeleton of the intersection complex together with the data of original maximal
		//cones containing these cones
		Matrix<Rational> interrays = f.rays;
		Matrix<Rational> interlin = f.lineality_space;
		int i_lineality_dim = rank(f.lineality_space);
		Vector<Set<int> > intercones;
		Vector<Set<int> > xcontainers;
		Vector<Set<int> > ycontainers;

		bool is_transversal = true;

		for(int ic = 0; ic < f.cones.rows(); ic++) {
			//Check that the cone dimension is at least the expected dimension
			int cone_dim = rank(interrays.minor(f.cones.row(ic),All)) + i_lineality_dim -1;
			if(cone_dim >= k) {
				if(cone_dim > k) {
					is_transversal = false;
					if(ensure_transversality) { 
						perl::ListReturn zeroResult;
						zeroResult << empty_cycle<Addition>(Xambi);
						zeroResult << false;
						return zeroResult;
					}
				}
				//Now we compute the k-skeleton of the intersection cone
				Vector<Set<int> > singlecone; singlecone |= f.cones.row(ic);
				IncidenceMatrix<> k_skeleton_matrix(singlecone);
				for(int i = cone_dim; i > k; i--) {
					k_skeleton_matrix = 
						calculateCodimOneData(interrays, k_skeleton_matrix, interlin, IncidenceMatrix<>()).codimOneCones;
				}

				//Go through all cones and add them (if they haven't already been added)
				for(int kc = 0; kc < k_skeleton_matrix.rows(); kc++) {
					int cone_index = -1;
					for(int oc = 0; oc < intercones.dim(); oc++) {
						//Since both cones have the same dimension, it suffices to check, whether the old cone
						//is contained in the new cone
						if((intercones[oc] * k_skeleton_matrix.row(kc)).size() == intercones[oc].size()) {
							cone_index = oc; break;
						}
					}
					//If it doesn't exist yet, add it
					if(cone_index == -1) {
						intercones |= k_skeleton_matrix.row(kc);
						xcontainers |= Set<int>();
						ycontainers |= Set<int>();
						cone_index = intercones.dim()-1;
					}

					//Now add containers
					xcontainers[cone_index] += f.xcontainers.row(ic);
					ycontainers[cone_index] += f.ycontainers.row(ic);

				}//END iterate all k-skeleton cones

			}//END if cone_dim >= k
		}//END iterate intersection cones

		//If no cones remain, return the zero cycle
		if(intercones.dim() == 0) {
			perl::ListReturn zeroResult;
			zeroResult << empty_cycle<Addition>(Xambi);
			zeroResult << false;
			return zeroResult;
		}

		//dbgtrace << "Computing weights " << endl;

		//Now we compute weights
		Vector<Integer> weights(intercones.dim());
		Set<int> weight_zero_cones;

		Matrix<Rational> xlin_dehom = xlin.minor(All,~scalar2set(0));
		Matrix<Rational> ylin_dehom = ylin.minor(All,~scalar2set(0));

		for(int c = 0; c < intercones.dim(); c++) {
			//dbgtrace << "Computing on intersection cone " << c << endl;
			//Find interior point
			Vector<Rational> interior_point = accumulate(rows(interrays.minor(intercones[c],All)),operations::add());
			Rational count_vertices = accumulate(interrays.col(0).slice(intercones[c]),operations::add());
			if(count_vertices != 0) interior_point /= count_vertices;

			//dbgtrace << "Interior point is " << interior_point << endl;

			//Compute stars
			Matrix<Rational> xstar_rays, ystar_rays;
			Vector<Set<int> > xstar_cones, ystar_cones;

			//dbgtrace << "Computing stars " << endl;

			computeStar(interior_point, xrays, xcones.minor(xcontainers[c],All), xstar_rays, xstar_cones);
			computeStar(interior_point, yrays, ycones.minor(ycontainers[c],All), ystar_rays, ystar_cones);

			//dbgtrace << "X Star rays: " << xstar_rays << endl;
			//dbgtrace << "X Star cones: " << xstar_cones << endl;
			//dbgtrace << "Y Star rays: " << ystar_rays << endl;
			//dbgtrace << "Y Star cones: " << ystar_cones << endl;

			//dbgtrace << "Computing multiplicity " << endl;

			Integer w = computeFanMultiplicity(
					xstar_rays, xlin_dehom, xstar_cones, xweights.slice(xcontainers[c]), Xdim,
					ystar_rays, ylin_dehom, ystar_cones, yweights.slice(ycontainers[c]), Ydim);

			//dbgtrace << "Weight is " << w << endl;

			weights[c] = w;
			if(w == 0) weight_zero_cones += c;

		}

		//Check if any cones remain
		if(weight_zero_cones.size() == intercones.dim()) {
			perl::ListReturn zeroResult;
			zeroResult << empty_cycle<Addition>(Xambi);
			zeroResult << false;
			return zeroResult;
		}

		//dbgtrace << "Done" << endl;

		//Clean up rays and cones

		IncidenceMatrix<> intercones_matrix(intercones);
		intercones_matrix = intercones_matrix.minor(~weight_zero_cones,All);
		Set<int> used_rays = accumulate(rows(intercones_matrix), operations::add());
		intercones_matrix = intercones_matrix.minor(All,used_rays);
		interrays = interrays.minor(used_rays,All);
		weights = weights.slice(~weight_zero_cones);

		//Finally create the result  
		perl::Object result(perl::ObjectType::construct<Addition>("Cycle"));
		result.take("VERTICES") << thomog(interrays);
		result.take("MAXIMAL_POLYTOPES") << intercones_matrix;
		result.take("LINEALITY_SPACE") << thomog(interlin);
		result.take("WEIGHTS") << weights;

		//     return result;
		perl::ListReturn positiveResult;
		positiveResult << result;
		positiveResult << is_transversal;
		return positiveResult;
	}



	// ------------------------- PERL WRAPPERS ---------------------------------------------------

	UserFunction4perl("# @category Lattices"
			"# This computes the index of a lattice in its saturation."
			"# @param Matrix<Integer> m A list of (row) generators of the lattice."
			"# @return Integer The index of the lattice in its saturation.",
			&lattice_index,"lattice_index(Matrix<Integer>)");

	UserFunctionTemplate4perl("# @category Intersection theory"
			"# Computes the intersection product of two tropical cycles in R^n and tests whether the intersection is transversal (in the sense that the cycles intersect set-theoretically in the right dimension)."
			"# @param Cycle X A tropical cycle"
			"# @param Cycle Y A tropical cycle, living in the same space as X"
			"# @param Bool ensure_transversality Whether non-transversal intersections should not be computed. Optional and false by default. If true,"
			"# returns the zero cycle if it detects a non-transversal intersection"
			"# @return List( Cycle intersection product, Bool is_transversal)."
                        "#  Intersection product is a zero cycle if ensure_transversality is true and the intersection is not transversal."
			"#  //is_transversal// is false if the codimensions of the varieties add up to more than the ambient dimension.",
			"intersect_check_transversality<Addition>(Cycle<Addition>,Cycle<Addition>; $=0)");

	InsertEmbeddedRule("# @category Intersection theory"
			"# Computes the intersection product of two tropical cycles in the projective torus"
			"# Use [[intersect_check_transversality]] to check for transversal intersections"
			"# @param Cycle X A tropical cycle"
			"# @param Cycle Y A tropical cycle, living in the same ambient space as X"
			"# @return Cycle The intersection product\n"
			"user_function intersect<Addition>(Cycle<Addition>,Cycle<Addition>) {\n"
			"	my ($X,$Y) = @_;\n"
			"	my @r = intersect_check_transversality($X,$Y);\n"
			"	return $r[0];\n"
			"}\n");

}}
