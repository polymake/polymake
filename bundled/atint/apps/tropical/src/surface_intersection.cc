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

	Contains functions to compute intersection products in tropical surfaces.
	*/

#include "polymake/client.h"
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/linalg.h"
#include "polymake/PowerSet.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/common/lattice_tools.h"
#include "polymake/tropical/thomog.h"
#include "polymake/tropical/linear_algebra_tools.h"
#include "polymake/tropical/lattice.h"
#include "polymake/tropical/specialcycles.h"
#include "polymake/tropical/convex_hull_tools.h"
#include "polymake/tropical/refine.h"
#include "polymake/tropical/morphism_thomog.h"

namespace polymake { namespace tropical {

	/*
	 * @brief Takes a list of rays which add up to 0, whose span dimension is one less than their number and whose maximal minors are 1. Then it also
	 * takes a list of vectors lying in that span. It will then compute for each vector the unique representation as a positive linear combination of the rays.
	 * @param Matrix<Rational> rank_one_flats, as PRIMITIVE INTEGER
	 * @param Matrix<Rational> curve_rays, as PRIMITIVE INTEGER
	 * @return Matrix<Integer> Each row corresponds to the representation of the same row in curve_rays. Each column corresponds to a row of surface_rays and contains
	 * the corresponding positive coefficient.
	 * */
	Matrix<Integer> positive_decomposition(const Matrix<Rational> &rank_one_flats, const Matrix<Rational> &curve_rays) {
		Matrix<Integer> result(curve_rays.rows(), rank_one_flats.rows());
		//Iterate curve rays
		for(int cr = 0; cr < curve_rays.rows(); cr++) {
			//Compute a linear representation of the vector in the rays
			Vector<Rational> linRep = linearRepresentation(curve_rays.row(cr), rank_one_flats);
			//Go through its entries. For each negative entry, add the absolute value to all other entries and set this one to 0.
			for(int entry = 0; entry < linRep.dim(); entry++) {
				if(linRep[entry] < 0) linRep = linRep - (linRep[entry] * ones_vector<Rational>(linRep.dim()));
			}
			result.row(cr) = Vector<Integer>(linRep);
		}
		return result;
	}//END positive_decomposition

	/*
	 * @brief Takes a list of rays of a curve given by their positive linear representations wrt to a rank-1-flat-vectors matrix and a list of weights of those rays. It then computes the degree of that curve.
	 * @param Matrix<Rational> curve_decompositions
	 * @param Vector<Integer> curve_weights
	 * @return Integer
	 */
	Integer degree_via_decomposition(const Matrix<Integer> &curve_decompositions, const Vector<Integer> &curve_weights) {
		Integer deg = 0;
		for(int i = 0; i < curve_decompositions.rows(); i++) {
			deg += curve_decompositions(i,0) * curve_weights[i];
		}
		return deg;
	}//END degree_in_uniform

	/*
	 * @brief Computes the intersection multiplicity of two fan curves in a surface that is GLnZ-isomorphic
	 * to a uniform surface
	 * @param Matrix<Integer> rank_one_flats An integer matrix, whose rows are rays in a surface corresponding to the rank one flats of a matroid realization of that surface.
	 * @param Matrix<Rational> curve_a_rays The rays of the first curve (not homog, no leading coord)
	 * @param Vector<Integer> curve_a_weights The weights of the rays of the first curve, in the same order as the rays
	 * @param Matrix<Rational> curve_b_rays The rays of the second curve (not homog, no leading coord)
	 * @param Vector<Integer> curve_b_weights The weights of the rays of the second curve, in the same order as the rays.
	 */
	Integer intersection_multiplicity_via_flats( Matrix<Rational> &rank_one_flats,
																 Matrix<Rational> &curve_a_rays,
																 const Vector<Integer> &curve_a_weights,
																 Matrix<Rational> &curve_b_rays,
																 const Vector<Integer> &curve_b_weights) {
		//Make everything integer
		rank_one_flats = Matrix<Rational>(common::primitive(rank_one_flats));
		curve_a_rays = Matrix<Rational>(common::primitive(curve_a_rays));
		curve_b_rays = Matrix<Rational>(common::primitive(curve_b_rays));

		Matrix<Integer> curve_a_decompositions = positive_decomposition( rank_one_flats, curve_a_rays);
		Matrix<Integer> curve_b_decompositions = positive_decomposition( rank_one_flats, curve_b_rays);

		Integer result = degree_via_decomposition( curve_a_decompositions, curve_a_weights) *
								degree_via_decomposition( curve_b_decompositions, curve_b_weights);


		//We iterate pairs of rays of the two curves
		for(int aray = 0; aray < curve_a_rays.rows(); aray++) {
			Vector<Integer> amap = curve_a_decompositions.row(aray);
			for(int bray = 0; bray < curve_b_rays.rows(); bray++) {
				Vector<Integer> bmap = curve_b_decompositions.row(bray);
				Integer correction = 0;
				//Iterate pairs of rank one flats
				for(pm::Subsets_of_k_iterator<const pm::Series<int,true> &> pair = entire(all_subsets_of_k( sequence(0,rank_one_flats.rows()),2)); !pair.at_end(); pair++) {
					Vector<int> pair_as_vector(*pair);
					correction = std::max( correction,
							std::min(
								amap[pair_as_vector[0]] * bmap[pair_as_vector[1]],
								amap[pair_as_vector[1]] * bmap[pair_as_vector[0]]
								)*curve_a_weights[aray]*curve_b_weights[bray]
							);
				}//END iterate pairs of rank one flats
				result -= correction;
			}//END iterate curve B rays
		}//END iterate curve A rays



		return result;
	}//END intersection_multiplicity_in_uniform

	/*
	 *	@brief Takes a smooth surface fan and finds vectors corresponding to the rank one flats in some matroidal
	 *	realization
	 * @param perl::Object surface A tropical surface fan.
	 *	@return Matrix<Rational> The rank one rays in non-tropically homogeneous coordinates.
	 */
	template <typename Addition>
        Matrix<Rational> find_rank_one_vectors(perl::Object surface)
        {
          perl::Object matroid, map;
          bool is_smooth;
          call_function("is_smooth", surface) >> is_smooth >> matroid >> map;
          //Sanity check
          if (!is_smooth) throw std::runtime_error("Finding rank one vectors: Surface is not smooth.");

          perl::Object face_lattice = matroid.give("LATTICE_OF_FLATS");

          //Extract rank one flats
          int N = matroid.give("N_ELEMENTS");
          NodeMap<Directed,Set<int>> all_faces = face_lattice.give("FACES");
          Set<int> rank_one_flats = face_lattice.call_method("nodes_of_rank", 1);

          Matrix<Rational> map_matrix = map.give("MATRIX");
          map_matrix = inv(map_matrix.minor(~scalar2set(0), ~scalar2set(0)));
          map_matrix = thomog_morphism(map_matrix, zero_vector<Rational>(map_matrix.cols())).first;

          Matrix<Rational> converted_vectors(0, map_matrix.cols());

          for (auto j = entire(rank_one_flats); !j.at_end(); ++j) {
            Vector<Rational> fvector(N);
            const Set<int>& flat = all_faces[*j];
            fvector.slice(flat) = Addition::orientation() * ones_vector<Rational>(flat.size());
            converted_vectors /= (map_matrix * fvector);
          }

          return tdehomog(converted_vectors, 0, 0);
        }//END find_rank_one_vectors

	/*
	 * @brief Finds all the maximal cones of a surface containing a given point
	 * @param Vector<Rational> point, in tropical homogeneous coordinates with leading coordinate.
	 * @param Matrix<Rational> facet_normals The [[FACET_NORMALS]]
	 * @param Matrix<Rational> affine_normals The [[AFFINE_HULL_NORMALS]]
	 * @param SparseMatrix<int> maximal_facets The [[MAXIMAL_POLYTOPES_FACETS]]
	 * @param IncidenceMatrix maximal_affine The [[MAXIMAL_POLYTOPES_AFFINE_HULL_NORMALS]]
	 * @param Matrix<Rational> vertices The [[VERTICES]]
	 * @param IncidenceMatrix<> cones The [[MAXIMAL_POLYTOPES]]
	 * @param Matrix<Rational> lineality The [[LINEALITY_SPACE]]
	 * @return perl::Object The star of the surface as a Cycle
	 */
	template <typename Addition>
		perl::Object compute_surface_star(const Vector<Rational> &point,
				const Matrix<Rational> &facet_normals,
				const Matrix<Rational> &affine_normals,
				const SparseMatrix<int> &maximal_facets,
				const IncidenceMatrix<> &maximal_affine,
				const Matrix<Rational> &vertices,
				const Matrix<Rational> &lineality,
				const IncidenceMatrix<> &cones) {

			//First of all find the set of all maximal cones containing the point.
			Set<int> containing_cones;
			for (int mc = 0; mc < cones.rows(); mc++) {
                                if (!is_zero(affine_normals.minor( maximal_affine.row(mc), All) * point)) {
					continue;
				}
				bool found_non_facet = false;
				for(int col = 0; col < maximal_facets.cols() && !found_non_facet; col++) {
					if(maximal_facets(mc,col) != 0) {
						if( facet_normals.row(col) * maximal_facets(mc,col) * point < 0) {
							found_non_facet = true; continue;
						}
					}
				}
				if(found_non_facet) continue;
				//Arriving here, it must contain the point
				containing_cones += mc;
			}//END iterate maximal cones

			//Now construct the star itself
			Matrix<Rational> star_rays = vertices;
			Matrix<Rational> star_lineality = lineality;
			IncidenceMatrix<> star_cones = cones;

			std::pair<Set<int>, Set<int> > f_and_nf = far_and_nonfar_vertices(vertices);
			Set<int> cone_intersection = accumulate( rows( cones.minor(containing_cones,All)), operations::mul());
			Set<int> unused_rays = sequence(0, vertices.rows()) -
				accumulate( rows(cones.minor(containing_cones,All)),operations::add());

			//Start by constructing the lineality space
			Vector<int> intersect_nonfar( cone_intersection * f_and_nf.second);
			Set<int> intersect_far = cone_intersection * f_and_nf.first;
			Set<int> rays_to_remove = intersect_far + unused_rays;

			for(int inf = 1; inf < intersect_nonfar.dim(); inf++) {
				star_lineality /= (vertices.row(intersect_nonfar[inf]) - star_rays.row(intersect_nonfar[0]));
				rays_to_remove += intersect_nonfar[inf];
			}
			star_lineality /= vertices.minor( intersect_far,All);
			star_lineality = star_lineality.minor( basis_rows(star_lineality),All);

			//Replace nonfar vertices in adjacent cones by differences
			Set<int> nonfar_remaining = f_and_nf.second - rays_to_remove - intersect_nonfar[0];
			for(Entire<Set<int> >::iterator nfr = entire(nonfar_remaining); !nfr.at_end(); nfr++) {
				star_rays.row(*nfr) = star_rays.row(*nfr) - star_rays.row( intersect_nonfar[0]);
			}

			//Make the reference vertex the origin
			star_rays.row( intersect_nonfar[0]) = unit_vector<Rational>( star_rays.cols(),0);

			perl::Object result(perl::ObjectType::construct<Addition>("Cycle"));
				result.take("VERTICES") << star_rays.minor(~rays_to_remove,All);
				result.take("MAXIMAL_POLYTOPES") << star_cones.minor(containing_cones, ~rays_to_remove);
				result.take("LINEALITY_SPACE") << star_lineality;
				result.take("WEIGHTS") << ones_vector<Integer>(containing_cones.size());
			return result;

		}//END findSurfaceStarCones


	/*
	 * @brief Computes the rays of the star of a curve at a point
	 * @param Matrix<Rational> curve_vertices The curve's vertices (non-homog. with leading coord)
	 * @param IncidenceMatrix<> curve_cones The cuve's cones
	 * @param IncidenceMatrix<> curve_containers The indices of all the maximal cones containing the point.
	 * @param Matrix<Rationa> lineality If the curve consists only of a lineality space, this is its generator.
	 * @return Matrix<Rational> The star rays, non-homog and without(!) leading coordinate.
	 */
	std::pair<Matrix<Rational>, Vector<Integer> > compute_curve_star_rays(
			const Matrix<Rational> &curve_vertices, const IncidenceMatrix<> &curve_cones,
			const Vector<Integer> &curve_weights, const Set<int> &curve_containers, const Matrix<Rational> &lineality ) {
		//Split up the rays in far and nonfar
		std::pair<Set<int>, Set<int> > f_and_nf = far_and_nonfar_vertices(curve_vertices);

		Matrix<Rational> result(0,curve_vertices.cols()-1);
		Vector<Integer> star_weights;

		//If the curve is only a lineality space:
		if(lineality.rows() > 0) {
			result /= lineality.minor(All,~scalar2set(0));
			result /= -lineality.minor(All,~scalar2set(0));
			star_weights |= curve_weights[0];
			star_weights |= curve_weights[0];
			return std::make_pair(result, star_weights);
		}

		//If there is only one maximal cone, we compute its direction vector
		if(curve_containers.size() == 1) {
			Set<int> single_cone = curve_cones.row( *(curve_containers.begin()));
			Set<int> far_in_container = f_and_nf.first * single_cone;
			Vector<Rational> direction;
			if(far_in_container.size() == 0) {
				//Its a bounded edge
				Vector<int> both_rays(single_cone);
				direction =
					curve_vertices.row( both_rays[0]) - curve_vertices.row(both_rays[1]);
			}
			else {
				direction = curve_vertices.row( *(far_in_container.begin()));
			}

			direction = direction.slice(~scalar2set(0));
			result /= direction;
			result /= -direction;
			star_weights = curve_weights[ *(curve_containers.begin())] * ones_vector<Integer>(2);
			return std::make_pair(result, star_weights);
		}
		else {
			//If its several, then every one of them must have point as a vertex
			int points_index = *( accumulate(rows(curve_cones.minor(curve_containers,All)), operations::mul()).begin());
			for(Entire<Set<int> >::const_iterator adjRays = entire(curve_containers); !adjRays.at_end(); adjRays++) {
				Vector<Rational> direction;
				int ray_index = *( (curve_cones.row(*adjRays) - points_index).begin());
				if(curve_vertices(ray_index,0) == 0) {
					direction = curve_vertices.row(ray_index);
				}
				else {
					direction = curve_vertices.row(ray_index) - curve_vertices.row(points_index);
				}
				direction = direction.slice(~scalar2set(0));
				result /= direction;
				star_weights |= curve_weights[*adjRays];
			}
		}
		return std::make_pair(result, star_weights);
	}//END compute_curve_star_rays

	/*
	 * @brief For a curve and a point, computes the indices of all maximal cones containing the point
	 * @param Vector<Rational> point the point, non-homog, with leading coord
	 * @param Matrix<Rational> curve_rays The curve vertices, non-homog, with leading coord
	 * @param IncidenceMatrix curve_cones The curve cones
	 * @param int some_container An index of one maximal cone containing the point
	 * @return Set<int> All indices of cones containing the point
	 */
	Set<int> compute_containing_cones(const Vector<Rational> &point, const Matrix<Rational> &curve_rays,
				const IncidenceMatrix<> &curve_cones, int some_container) {
		//Go through the rays of the container - if point is equal to one of them, return all
		//maximal cones using the vertex
		Set<int> container_set = curve_cones.row(some_container);
		for(Entire<Set<int> >::iterator cray = entire(container_set); !cray.at_end(); cray++) {
			if(point == curve_rays.row(*cray)) {
				return T(curve_cones).row(*cray);
			}
		}

		//If it isn't, it's just the one cone
		return scalar2set(some_container);
	}


	template <typename Addition>
		perl::Object intersect_in_smooth_surface(perl::Object surface, perl::Object cycle_a, perl::Object cycle_b) {
			//Extract data
			int dim_a = cycle_a.give("PROJECTIVE_DIM");
			int dim_b = cycle_b.give("PROJECTIVE_DIM");
			int ambient_dim = surface.give("PROJECTIVE_AMBIENT_DIM");

			//Basic sanity checks
			if(dim_a + dim_b <= 1) return empty_cycle<Addition>(ambient_dim);
			if(dim_a > 2 || dim_b > 2) throw std::runtime_error("intersect_in_smooth_surface: Cycles dimension too large.");

			//If one is full-dimensional, return the other one with multiplied weights
			Vector<Integer> weights_a = cycle_a.give("WEIGHTS");
			Vector<Integer> weights_b = cycle_b.give("WEIGHTS");
			if (dim_a == 2) {
				return cycle_b.call_method("multiply_weights", weights_a[0]);
			}
			if (dim_b == 2) {
				return cycle_a.call_method("multiply_weights", weights_b[1]);
			}

			//From here on we know we have two curves.

			//Refine both curves along the surface
			RefinementResult refined_cycle_a = refinement( cycle_a, surface, false, false, false, true, false);
			RefinementResult refined_cycle_b = refinement( cycle_b, surface, false, false, false, true, false);
			Matrix<Rational> rays_a = refined_cycle_a.complex.give("VERTICES");
				rays_a = tdehomog(rays_a);
			Matrix<Rational> rays_b = refined_cycle_b.complex.give("VERTICES");
				rays_b = tdehomog(rays_b);
			Matrix<Rational> lin_a = refined_cycle_a.complex.give("LINEALITY_SPACE");
				lin_a = tdehomog(lin_a);
			Matrix<Rational> lin_b = refined_cycle_b.complex.give("LINEALITY_SPACE");
				lin_b = tdehomog(lin_b);
			IncidenceMatrix<> cones_a = refined_cycle_a.complex.give("MAXIMAL_POLYTOPES");
			IncidenceMatrix<> cones_b = refined_cycle_b.complex.give("MAXIMAL_POLYTOPES");
			Vector<Integer> refweights_a = refined_cycle_a.complex.give("WEIGHTS");
			Vector<Integer> refweights_b = refined_cycle_b.complex.give("WEIGHTS");

			//Now intersect the refined versions.
			fan_intersection_result ab_intersect = cdd_fan_intersection(
					rays_a, lin_a, cones_a, rays_b, lin_b, cones_b);


			//The potential intersection points are the nonfar vertices of this
			Matrix<Rational> ab_vertices = ab_intersect.rays;
			Matrix<Rational> ab_vertices_homog = thomog(ab_vertices);
			IncidenceMatrix<> ab_vertices_by_cones = T(ab_intersect.cones);
			Vector<Integer> ab_weights;
			Set<int> nonfar_and_nonzero;

			Matrix<Rational> surface_rays = surface.give("VERTICES");
			IncidenceMatrix<> surface_cones = surface.give("MAXIMAL_POLYTOPES");
			Matrix<Rational> surface_lineality = surface.give("LINEALITY_SPACE");
			Matrix<Rational> facet_normals = surface.give("FACET_NORMALS");
			Matrix<Rational> affine_hull = surface.give("AFFINE_HULL");
			SparseMatrix<int> maximal_facets = surface.give("MAXIMAL_POLYTOPES_FACETS");
			IncidenceMatrix<> maximal_affine = surface.give("MAXIMAL_POLYTOPES_AFFINE_HULL_NORMALS");

			for(int point = 0; point < ab_vertices.rows(); point++) {
				//If it's a ray, ignore it.
				if(ab_vertices(point,0) == 0) {
					continue;
				}

				//If it's a vertex, we need to compute the star of X at that point.
				perl::Object surface_star = compute_surface_star<Addition>(
						ab_vertices_homog.row(point),
						facet_normals,
						affine_hull,
						maximal_facets,
						maximal_affine,
                                                surface_rays,
						surface_lineality,
						surface_cones);
				Matrix<Rational> rank_one_vectors = find_rank_one_vectors<Addition>(surface_star);

				//Now compute the star at each curve
				int cone_of_point = *(ab_vertices_by_cones.row(point).begin());
				int cone_of_a = *(ab_intersect.xcontainers.row(cone_of_point).begin());
				int cone_of_b = *(ab_intersect.ycontainers.row(cone_of_point).begin());
				std::pair<Matrix<Rational>, Vector<Integer> > a_star =
					compute_curve_star_rays(rays_a, cones_a, refweights_a,
						compute_containing_cones( ab_vertices.row(point), rays_a, cones_a, cone_of_a), lin_a);
				std::pair<Matrix<Rational>, Vector<Integer> > b_star =
					compute_curve_star_rays(rays_b, cones_b, refweights_b,
						compute_containing_cones( ab_vertices.row(point), rays_b, cones_b, cone_of_b), lin_b);


				//Finally, we can compute the multiplicity

				Integer mult = intersection_multiplicity_via_flats(
						rank_one_vectors, a_star.first, a_star.second, b_star.first, b_star.second);


				if(mult != 0) {
					nonfar_and_nonzero += point;
					ab_weights |= mult;
				}
			}//END iterate intersection rays

			//If no points remain, return the empty_cycle
			if(nonfar_and_nonzero.size() == 0) return empty_cycle<Addition>(ambient_dim);
			ab_vertices = ab_vertices.minor(nonfar_and_nonzero,~scalar2set(0));

			return point_collection<Addition>( thomog(ab_vertices,0,0), ab_weights);
		}//END intersect_in_smooth_surface


	// --------------------- PERL WRAPPERS -----------------------------


	UserFunctionTemplate4perl("# @category Intersection theory"
                                  "# Computes the intersection product of two cycles in a smooth surface"
                                  "# @param Cycle<Addition> surface A smooth surface"
                                  "# @param Cycle<Addition> A any cycle in the surface"
                                  "# @param Cycle<Addition> B any cycle in the surface"
                                  "# @return Cycle<Addition> The intersection product of A and B in the surface",
                                  "intersect_in_smooth_surface<Addition>(Cycle<Addition>,Cycle<Addition>, Cycle<Addition>)");

	FunctionTemplate4perl("compute_surface_star<Addition>(Vector, Matrix,Matrix,SparseMatrix<Int>, IncidenceMatrix, Matrix, Matrix,IncidenceMatrix)");

}}
