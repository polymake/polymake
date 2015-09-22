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

	Contains functions to compute tropical Hurwitz cycles.
	*/

#include "polymake/client.h"
#include "polymake/Matrix.h"
#include "polymake/Rational.h"
#include "polymake/Vector.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/PowerSet.h"
#include "polymake/permutations.h"
#include "polymake/linalg.h"
#include "polymake/tropical/morphism_special.h"
#include "polymake/tropical/specialcycles.h"
#include "polymake/tropical/refine.h"
#include "polymake/tropical/convex_hull_tools.h"
#include "polymake/tropical/moduli_rational.h"
#include "polymake/tropical/solver_def.h"
#include "polymake/tropical/LoggingPrinter.h"
#include "polymake/tropical/thomog.h"
#include "polymake/tropical/morphism_thomog.h"
#include "polymake/TropicalNumber.h"
#include "polymake/tropical/misc_tools.h"

namespace polymake { namespace tropical {

	using namespace atintlog::donotlog;
	//using namespace atintlog::dolog;
	   //using namespace atintlog::dotrace;

	struct HurwitzResult {
		perl::Object subdivision;
		perl::Object cycle;
	};

	///////////////////////////////////////////////////////////////////////////////////////

	/**
	  @brief Takes a RationalCurve and a list of node indices. Then inserts additional leaves (starting from N_LEAVES+1) at these nodes and returns the resulting RationalCurve object
	  @param perl::Object curve A RationalCurve object
	  @param Vector<int> nodes A list of node indices of the curve
	  */
	perl::Object insert_leaves(perl::Object curve, Vector<int> nodes) {
		//Extract values
		int max_leaf = curve.give("N_LEAVES");
		IncidenceMatrix<> setsInc = curve.give("SETS");
		Vector<Set<int> > sets = incMatrixToVector(setsInc);
		Vector<Rational> coeffs = curve.give("COEFFS");
		IncidenceMatrix<> nodes_by_sets = curve.give("NODES_BY_SETS");
		IncidenceMatrix<> nodes_by_leavesInc = curve.give("NODES_BY_LEAVES");
		Vector<Set<int> > nodes_by_leaves = incMatrixToVector(nodes_by_leavesInc);

		for(int n_el = 0; n_el < nodes.dim(); n_el++) {
			int n = nodes[n_el];
			max_leaf++;
			//Easy case: There is already a leaf at this vertex
			if(nodes_by_leaves[n].size() > 0) {
				int ref_leaf = *( nodes_by_leaves[n].begin());
				for(int s = 0; s < sets.dim(); s++) {
					if(sets[s].contains(ref_leaf)) {
						sets[s] += max_leaf;
					}
				}
			}//END if there is already a leaf
			else {
				//Normalize the sets at the node to point away from it. Order them in an arbitrary way
				//Intersect the first two, I_1 and I_2: I_1 points away from the node, if and only if
				//I_1 cap I_2 = empty or I_1. The subsequent sets I_k only have to fulfill 
				//I_k cap I_1 = empty
				Vector<int> adjacent_sets(nodes_by_sets.row(n));
				Vector<Set<int> > normalized_sets;
				Set<int> first_inter = sets[adjacent_sets[0]] * sets[adjacent_sets[1]];
				normalized_sets |= 
					(first_inter.size() == 0 || first_inter.size() == sets[adjacent_sets[0]].size()) ? sets[adjacent_sets[0]] : (sequence(1, max_leaf-1) - sets[adjacent_sets[0]]);
				for(int as = 1; as < adjacent_sets.dim(); as++) {
					Set<int> subseq_inter = normalized_sets[0] * sets[adjacent_sets[as]];
					normalized_sets |=
						(subseq_inter.size() == 0)? sets[adjacent_sets[as]] : (sequence(1,max_leaf-1) - sets[adjacent_sets[as]]);
				}
				//Now for each set count, how many of the adjacent sets intersect it nontrivially
				//It points away from the node, if and only if this number is 1 (otherwise its all)
				for(int s = 0; s < sets.dim(); s++) {
					int inter_count = 0;
					for(int ns = 0; ns < normalized_sets.dim(); ns++) {
						if( (sets[s] * normalized_sets[ns]).size() > 0) inter_count++;
						if(inter_count > 1) break;
					}
					if(inter_count > 1) {
						sets[s] += max_leaf; 
					}	    
				}
			}//END if there are only bounded edges

			nodes_by_leaves[n] += max_leaf;
		}//END iterate nodes

		//Create result
		perl::Object result("RationalCurve");
		result.take("SETS") << sets;
		result.take("COEFFS") << coeffs;
		result.take("N_LEAVES") << max_leaf;
		return result;
    
  }//END insert_leaves

	///////////////////////////////////////////////////////////////////////////////////////

	/**
	  @brief Takes a RationalCurve object and returns the rays v_I corresponding to its bounded edges as a matrix of row vectors in matroid coordinates (with leading coordinate 0).
	  @param perl::Object curve A RationalCurve object
	  @return Matrix<Rational> The rays corresponding to the bounded edges
	  */
	template <typename Addition>
		Matrix<Rational> edge_rays(perl::Object curve) {
			IncidenceMatrix<> sets = curve.give("SETS");
			int n = curve.give("N_LEAVES");
			Matrix<Rational> result(0, n*(n-3)/2 + 2);
			for(int s = 0; s < sets.rows(); s++) {
				perl::Object rcurve("RationalCurve");
				rcurve.take("SETS") << sets.minor(scalar2set(s),All);
				rcurve.take("N_LEAVES") << n;
				rcurve.take("COEFFS") << ones_vector<Rational>(1);
				Vector<Rational> rray = CallPolymakeFunction("matroid_vector",rcurve, Addition());
				result /= rray;
			}
			return result;
		}
	///////////////////////////////////////////////////////////////////////////////////////

	/**
	  @brief Computes all ordered lists of k elements in {0,...,n-1}
	  @param int n The size of the set {0,...,n-1}
	  @param int k The size of the ordered subsets
	  @return Matrix<int> All ordered k-sets as a list of row vectors, i.e. a matrix of dimension ( (n choose k) * k!) x k
	  */
	Matrix<int> ordered_k_choices(int n, int k) {

		Matrix<int> result(0,k);

		//Compute all k-choices of the set {0,..,n-1}
		Array<Set<int> > kchoices = all_subsets_of_k(sequence(0,n),k);

		//Compute all permutations on a k-set
		AllPermutations<> kperm = all_permutations(k);

		for(int ch = 0; ch < kchoices.size(); ch++) {
			Vector<int> kvec(kchoices[ch]);
			for(Entire<AllPermutations<> >::iterator p = entire(kperm);!p.at_end(); p++) {
				result /= (permuted(kvec,*p));
			}
		}

		return result;
	}//END ordered_k_choices

	///////////////////////////////////////////////////////////////////////////////////////

	/**
	  @brief Takes a matrix and computes the gcd of the absolute values of the maximal minors
	  */
	Integer gcd_maxminor(Matrix<Rational> map) {
		Array<Set<int> > r_choices = all_subsets_of_k(sequence(0,map.cols()),map.rows());
		int g = 0;
		for(int r = 0; r < r_choices.size(); r++) {
			g = gcd(g,det(map.minor(All,r_choices[r])).to_int());
		}
		return Integer(abs(g));
	}

	///////////////////////////////////////////////////////////////////////////////////////

	/**
	  @brief This takes a list of cones (in terms of ray indices) and their weights and inserts another cone with given weight. If the new cone already exists in the list, the weight is added
	  */
	void insert_cone(Vector<Set<int> > &cones, Vector<Integer> &weights, Set<int> ncone, Integer nweight) {
		int ncone_index = -1;
		for(int c = 0; c < cones.dim(); c++) {
			Set<int> inter = ncone * cones[c];
			if( inter.size() == ncone.size() && inter.size() == cones[c].size()) {
				ncone_index = c; break;
			}
		}
		if(ncone_index == -1) {
			cones |= ncone;
			weights |= nweight;
		}
		else {
			if(weights.dim() > ncone_index) weights[ncone_index] += nweight;
		}
	}

	///////////////////////////////////////////////////////////////////////////////////////

	/**
	  @brief Checks whether the cone of the positive orthant intersects all of the hyperplanes defined by ev_maps.row(i) = p_i in its interior (i.e. is not contained in one of the half spaces >= pi / <= pi), where p_i = 1,2,... This can be used to check whether a choice of fixed vertices is valid (because the corresponding refinement has to have at least one interior cone for a generic choice of points p_i). In fact, this is equivalent to saying that each row of the matrix has at least one positive entry and that the hyperplanes intersect the orthant in a cone, s.t. for each entry _j there is at least one ray or vertex v s.t. v_j > 0.
	  */
	bool is_valid_choice(const Matrix<Rational> &ev_maps, solver<Rational> &sv) {
		//Check if all maps have a positive entry.
		for(int r = 0; r < ev_maps.rows(); r++) {
			bool found_positive = false;
			for(int c = 0; c < ev_maps.cols(); c++) {
				if(ev_maps(r,c) > 0) {
					found_positive = true; break;
				}
			}
			if(!found_positive) return false;
		}

		//Now compute the intersection of all hyperplanes with the positive orthant
		Matrix<Rational> ineq = unit_matrix<Rational>(ev_maps.cols());
		ineq = zero_vector<Rational>() | ineq;
		Matrix<Rational> eq = ev_maps;
		eq = (- Vector<Rational>(sequence(1,ev_maps.rows()))) | eq;
		try {
			Matrix<Rational> rays = sv.enumerate_vertices(ineq,eq, false,true).first;
			bool found_positive = false;
			for(int c = 1; c < rays.cols(); c++) {
				for(int r = 0; r < rays.rows(); r++) {
					if(rays(r,c) > 0) {
						found_positive = true; break;
					}
				}
				if(!found_positive) return false;
			}
		}
		catch(...) {
			//This might happen, if the system has no solution
			return false;
		}
		return true;
	}

	///////////////////////////////////////////////////////////////////////////////////////

	/**
	  @brief Computes a subdivision of M_0,n containing H_k(degree) and (possibly) the cycle itself. The function returns a struct containing the subdivision as subobject "subdivision" and the Hurwitz cycle as subobject "cycle", both as perl::Object. Optionally one can specify a RationalCurve object representing a ray around which the whole computation is performed locally (the vector is only for default initalization).
	  */
	template <typename Addition>
		HurwitzResult hurwitz_computation(int k, Vector<int> degree, Vector<Rational> points, bool compute_cycle, std::vector<perl::Object> local_restriction, bool output_progress) {

			solver<Rational> sv;

			//Make points default points ( = 0) 
			if(points.dim() < degree.dim() - 3- k) {
				points = points | zero_vector<Rational>( degree.dim()-3-k-points.dim());
			}
			if(points.dim() > degree.dim() - 3 - k) {
				points = points.slice(sequence(0,degree.dim()-3-k));
			}

			int n = degree.dim();
			int big_n = 2*n - k -2;
			int big_moduli_dim = big_n * (big_n - 3) / 2;//Affine ambient dimension of the stable maps. 

			//Compute M_0,n and extract cones
			perl::Object m0n;
			Vector<Rational> compare_vector;
			bool restrict_local = false;

			if(local_restriction.size() == 0) {
				m0n = CallPolymakeFunction("m0n_wrap",n,Addition());
			}
			else {
				perl::Object lr_ray = local_restriction[0];
				m0n = CallPolymakeFunction("local_m0n_wrap", Addition(), lr_ray); 
				CallPolymakeFunction("matroid_vector",lr_ray,Addition()) >> compare_vector;
				//We need to dehomogenize to make comparison possible.
				compare_vector = tdehomog_vec(compare_vector);
				restrict_local = true;	
			}

			IncidenceMatrix<> mn_cones = m0n.give("MAXIMAL_POLYTOPES");
			IncidenceMatrix<> mn_restrict;
			if(m0n.exists("LOCAL_RESTRICTION")) {
				m0n.give("LOCAL_RESTRICTION") >> mn_restrict;
			}
			/*int restrict_index = -1;
			if(restrict_local) {
				restrict_index = *(mn_restrict.row(0).begin());
				//dbgtrace << "Restricting at ray " << restrict_index << endl;
			}*/


			//Create evaluation maps
			Matrix<Rational> ev_maps(0, big_moduli_dim);
			Matrix<Rational> rat_degree(n,0);
			rat_degree |= degree;
			Vector<Rational> zero_translate(2);
			for(int i = n+2; i <= 2*n-2-k; i++) {
				perl::Object evi = evaluation_map<Addition>(n-2-k, thomog(rat_degree,0,false), i-n-1);
				Matrix<Rational> evimatrix = evi.give("MATRIX");
				//We take the representation of the evaluation map on charts 0 and 0.
				evimatrix = tdehomog_morphism(evimatrix, zero_translate).first;
				// ... and forget the xR-coordinate, as it is assumed to be always 0.
				ev_maps /= (evimatrix.row(0).slice(sequence(0, evimatrix.cols()-1)));
			}

			//Affine ambient dim of m0n
			int mn_ambient_dim = n*(n-3)/2;
			//Will contain the rays/cones of the subdivided M_0,n (with leading coordinates)
			Matrix<Rational> subdiv_rays(0,mn_ambient_dim);
			Vector<Set<int> > subdiv_cones;
			Vector<Integer> subdiv_weights;
			//     Vector<Set<int> > subdiv_local;

			//Will contain the rays/cones/weights of the Hurwitz cycle
			Matrix<Rational> cycle_rays(0,mn_ambient_dim);
			Vector<Set<int> > cycle_cones;
			Vector<Integer> cycle_weights;
			//     Vector<Set<int> > cycle_local;

			//Iterate all cones of M_0,n and compute their refinement as well as the 
			//Hurwitz cycle cones within
			for(int mc = 0; mc < mn_cones.rows(); mc++) {
				if(output_progress)
					pm::cout << "Refining cone " << mc << " of " << mn_cones.rows() << endl;

				//Extract the combinatorial type to compute evaluation maps
				perl::Object mc_type = CallPolymakeFunction("rational_curve_from_cone",m0n,degree.dim(),mc);
				std::string mcs = mc_type.CallPolymakeMethod("to_string");
				//dbgtrace << "Maximal cone " << mcs << endl;

				//This will be a model of the subdivided cone of M_0,n
				Matrix<Rational> model_rays = unit_matrix<Rational>(degree.dim()-2);
				Vector<Set<int> > model_cones; model_cones |= sequence(0, degree.dim()-2);
				//Translate the local restriction if necessary
				Vector<Set<int> > model_local_restrict;
				if(restrict_local) {
					Set<int> single_index_set;
					Matrix<Rational> erays = edge_rays<Addition>(mc_type);
					for(int er = 0; er < erays.rows(); er++) {
						if(tdehomog_vec(Vector<Rational>(erays.row(er))) == compare_vector) {
							single_index_set += er; break;
						}
					}
					model_local_restrict |= single_index_set;
				}

				perl::Object model_complex(perl::ObjectType::construct<Addition>("Cycle"));
				model_complex.take("VERTICES") << thomog(model_rays);
				model_complex.take("MAXIMAL_POLYTOPES") << model_cones;
				if(restrict_local) {
					model_complex.take("LOCAL_RESTRICTION") << model_local_restrict;
				}

				//Iterate over all possible ordered choices of (#vert-k)-subsets of nodes  
				Matrix<int> fix_node_sets = ordered_k_choices(n-2, n-2-k);
				//dbgtrace << "Iterating all choices of fixed vertices " << fix_node_sets << endl;

				//We save the evaluation matrices for use in the computation
				//of tropical weights in the Hurwitz cycle
				Vector<Matrix<Rational> > evmap_list;

				for(int nchoice = 0 ; nchoice < fix_node_sets.rows(); nchoice++) {
					//Compute combinatorial type obtained by adding further ends to chosen nodes
					perl::Object higher_type = insert_leaves(mc_type, fix_node_sets.row(nchoice));
					std::string s = higher_type.CallPolymakeMethod("to_string");
					//dbgtrace << "Intersecting with type " << s << endl;
					//Convert evaluation maps to local basis of rays
					Matrix<Rational> local_basis = tdehomog(edge_rays<Addition>(higher_type)).minor(All,~scalar2set(0));
					Matrix<Rational> converted_maps = ev_maps * T(local_basis);

					//Check if this choice of fixed vertices induces some valid Hurwitz type
					//(in the generic case)
					//dbgtrace << "Checking validity..." << endl;
					if(is_valid_choice(converted_maps, sv)) {
						//dbgtrace << "Is valid" << endl;
						evmap_list |= converted_maps;
					}

					//dbgtrace << "Computing halfspace intersections"  << endl;
					//Now refine along each evaluation map
					for(int evmap = 0; evmap < converted_maps.rows(); evmap++) {
						//Compute half-space fan induced by the equation ev_i >= / = / <= p_i
						//Then refine the model cone along this halfspace
						if(converted_maps.row(evmap) != zero_vector<Rational>(converted_maps.cols())) {
							//Create homogenized version of evaluation map 
							Vector<Rational> hom_converted_map = converted_maps.row(evmap);
							hom_converted_map = ( - ones_vector<Rational>(hom_converted_map.dim()) * hom_converted_map) | hom_converted_map;

							perl::Object evi_halfspace = halfspace_subdivision<Addition>(points[evmap],hom_converted_map,1);
							model_complex = refinement(model_complex, evi_halfspace, false,false,false,true,false).complex;
						}
					}//END iterate evaluation maps

				}//END iterate choices of fixed nodes

				//If we are actually computing the Hurwitz cycle, we take the evalutation map
				//corresponding to each choice of vertex placement. We then compute the k-cones
				//in the subdivision on which this map is (p_1,..,p_r) and add the gcd of the maximal minors of the
				//matrix as tropical weight to these cones
				//dbgtrace << "Computing weights " << endl;
				IncidenceMatrix<> model_hurwitz_cones;
				Vector<Integer> model_hurwitz_weights;
				Matrix<Rational> model_hurwitz_rays;
				Vector<Set<int> > model_hurwitz_local;
				if(compute_cycle) {
					perl::Object skeleton = CallPolymakeFunction("skeleton_complex",model_complex,k,false);
					//We go through all dimension - k cones of the subdivision
					Matrix<Rational> k_rays = skeleton.give("VERTICES");
					k_rays = tdehomog(k_rays);
					IncidenceMatrix<> k_cones = skeleton.give("MAXIMAL_POLYTOPES");
					model_hurwitz_weights = zero_vector<Integer>(k_cones.rows());
					//dbgtrace << "Rays: " << k_rays << endl;
					//dbgtrace << "Cones: " << k_cones << endl;
					Set<int> non_zero_cones;
					for(int m = 0; m < evmap_list.dim(); m++) {
						//Compute gcd of max-minors
						//dbgtrace << "Matrix is " << evmap_list[m] << endl;
						Integer g = gcd_maxminor(evmap_list[m]);
						for(int c = 0; c < k_cones.rows(); c++) {
							//Check if ev maps to the p_i on this cone (rays have to be mapped to 0!)
							Set<int> cset = k_cones.row(c);
							bool maps_to_pi = true;
							for(Entire<Set<int> >::iterator cit = entire(cset);!cit.at_end(); cit++) {
								Vector<Rational> cmp_vector = 
									( k_rays(*cit,0) == 1? points : zero_vector<Rational>(points.dim())) ;
								if( evmap_list[m] * k_rays.row(*cit).slice(~scalar2set(0)) != cmp_vector ){
									maps_to_pi = false;break;
								}
							}
							if(maps_to_pi) {
								model_hurwitz_weights[c] += g;
								if(g != 0) non_zero_cones += c;
								//dbgtrace << "Adding weight to cone " << c << endl;
							}
						}
					}//END iterate evaluation maps
					//Identify cones with non-zero weight and the rays they use
					Set<int> used_rays = accumulate(rows(k_cones.minor(non_zero_cones,All)),operations::add());
					model_hurwitz_rays = Matrix<Rational>(k_rays.minor(used_rays,All));
					model_hurwitz_cones = IncidenceMatrix<>(k_cones).minor(non_zero_cones,used_rays);
					model_hurwitz_weights = model_hurwitz_weights.slice(non_zero_cones);

					//dbgtrace << "Model weight: " << model_hurwitz_weights << endl;

				}//END compute hurwitz weights

				//dbgtrace << "Re-converting refined cone " << endl;

				//Finally convert the model cones back to M_0,n-coordinates

				Matrix<Rational> model_subdiv_rays = model_complex.give("VERTICES");
				model_subdiv_rays = tdehomog(model_subdiv_rays);
				IncidenceMatrix<> model_subdiv_cones = model_complex.give("MAXIMAL_POLYTOPES");
				// 	Vector<Set<int> > model_subdiv_local = model_complex.give("LOCAL_RESTRICTION");
				Vector<Integer> model_subdiv_weights = ones_vector<Integer>(model_subdiv_cones.rows());
				//If we want to compute the Hurwitz cycle, we take the k-cones instead

				for(int i = 0; i <= 1; i++) {
					if(i == 1 && !compute_cycle) break;
					Matrix<Rational> rays_to_convert = (i==0? model_subdiv_rays : model_hurwitz_rays);
					IncidenceMatrix<> cones_to_convert = (i == 0? model_subdiv_cones: model_hurwitz_cones);
					Vector<Integer> weights_to_convert = (i == 0? model_subdiv_weights : model_hurwitz_weights);
					// 	  Vector<Set<int> > local_to_convert = (i == 0? model_subdiv_local : model_hurwitz_local);
					//First convert the rays back
					//dbgtrace << "Final rays are: " << model_subdiv_rays << endl;

					Matrix<Rational> model_conv_rays = 
						rays_to_convert.col(0) | 
						(rays_to_convert.minor(All,~scalar2set(0)) * 
						 tdehomog(edge_rays<Addition>(mc_type)).minor(All,~scalar2set(0)));
					//dbgtrace << "Converted rays are: " << model_conv_rays << endl;
					Vector<int> model_rays_perm  = insert_rays(i == 0? subdiv_rays : cycle_rays, model_conv_rays,false);
					Map<int,int> ray_index_map;
					for(int mrp = 0; mrp < model_rays_perm.dim(); mrp++) {
						ray_index_map[mrp] = model_rays_perm[mrp];
					}
					//Then use the above index list to transform the cones
					for(int msc = 0; msc < cones_to_convert.rows(); msc++) {
						insert_cone(i == 0? subdiv_cones : cycle_cones, 
								i == 0? subdiv_weights : cycle_weights, 
								attach_operation(cones_to_convert.row(msc), pm::operations::associative_access<Map<int,int>,int>(&ray_index_map)), 
								weights_to_convert[msc]);
					}
					//Do the same for the local restriction
					// 	  for(int lsc = 0; lsc < local_to_convert.dim(); lsc++) {
					// 	    Vector<Integer> dummy;
					// 	    insert_cone(i == 0? subdiv_local : cycle_local,
					// 			dummy, 
					// 			attach_operation(local_to_convert[lsc],
					// 			pm::operations::associative_access<Map<int,int>,int>(&ray_index_map)),0);
					//  		      
					// 	  }
				}

			}//END iterate cones of M_0,n

			//Finally find the localizing ray in both complexes
			Set<int> subdiv_local;
			Set<int> cycle_local;
			if(restrict_local) {
				for(int sr = 0; sr < subdiv_rays.rows(); sr++) {
					if(subdiv_rays.row(sr) == compare_vector) {
						subdiv_local += sr;break;
					}
				}
				for(int cr = 0; cr < cycle_rays.rows(); cr++) {
					if(cycle_rays.row(cr) == compare_vector) {
						cycle_local += cr;break;
					}
				}

				//Find vertex
				for(int sr = 0; sr < subdiv_rays.rows(); sr++) {
					if(subdiv_rays.row(sr) == unit_vector<Rational>(subdiv_rays.cols(),0)) {
						subdiv_local += sr;break;
					}
				}
				for(int cr = 0; cr < cycle_rays.rows(); cr++) {
					if(cycle_rays.row(cr) == unit_vector<Rational>(cycle_rays.cols(),0)) {
						cycle_local += cr;	break;
					}
				}
			}//END restrict

			perl::Object result(perl::ObjectType::construct<Addition>("Cycle"));
			result.take("VERTICES") << thomog(subdiv_rays);
			result.take("MAXIMAL_POLYTOPES") << subdiv_cones;
			result.take("WEIGHTS") << subdiv_weights;
			if(restrict_local) {
				Vector<Set<int> > subdiv_loc_inc; subdiv_loc_inc |= subdiv_local;
				CallPolymakeFunction("local_restrict",result,IncidenceMatrix<>(subdiv_loc_inc)) >> result;
			}

			perl::Object cycle(perl::ObjectType::construct<Addition>("Cycle"));
			cycle.take("VERTICES") << thomog(cycle_rays);
			cycle.take("MAXIMAL_POLYTOPES") << cycle_cones;
			cycle.take("WEIGHTS") << cycle_weights;
			if(restrict_local) {
				Vector<Set<int> > cycle_loc_inc; cycle_loc_inc |= cycle_local;
				CallPolymakeFunction("local_restrict",cycle,IncidenceMatrix<>(cycle_loc_inc)) >> cycle;
			}

			HurwitzResult final_result;
			final_result.subdivision = result;
			final_result.cycle = cycle;

			return final_result;
		}//END hurwitz_computation

	///////////////////////////////////////////////////////////////////////////////////////

	//Documentation see perl wrapper
	template <typename Addition>
		perl::Object hurwitz_subdivision(int k, Vector<int> degree, Vector<Rational> points, perl::OptionSet options) {
			return hurwitz_computation<Addition>(k,degree,points,false, std::vector<perl::Object>(), options["Verbose"]).subdivision;
		}

	//Documentation see perl wrapper
	template <typename Addition>
		perl::Object hurwitz_cycle(int k,  Vector<int> degree, Vector<Rational> points, perl::OptionSet options) {
			return hurwitz_computation<Addition>(k,degree,points,true, std::vector<perl::Object>(), options["Verbose"]).cycle;
		}

	//Documentation see perl wrapper
	template <typename Addition>
		perl::ListReturn hurwitz_pair(int k,  Vector<int> degree, Vector<Rational> points, perl::OptionSet options) {
			HurwitzResult r = hurwitz_computation<Addition>(k,degree,points,true, std::vector<perl::Object>(), options["Verbose"]);
			perl::ListReturn l;
			l << r.subdivision;
			l << r.cycle;
			return l;
		}

	//Documentation see perl wrapper
	template<typename Addition>
		perl::ListReturn hurwitz_pair_local(int k, Vector<int> degree, perl::Object local_restriction, perl::OptionSet options) {
			std::vector<perl::Object> ray_to_list;
			ray_to_list.push_back(local_restriction);
			HurwitzResult r = hurwitz_computation<Addition>(k,degree, Vector<Rational>(), true,ray_to_list, options["Verbose"]);
			perl::ListReturn l;
			l << r.subdivision;
			l << r.cycle;
			return l;
		}

	UserFunctionTemplate4perl("# @category Hurwitz cycles"
			"# This function computes a subdivision of M_0,n containing the Hurwitz cycle"
			"# H_k(x), x = (x_1,...,x_n) as a subfan. If k = n-4, this subdivision is the unique"
			"# coarsest subdivision fulfilling this property"
			"# @param Int k The dimension of the Hurwitz cycle, i.e. the number of moving vertices"
			"# @param Vector<Int> degree The degree x. Should add up to 0"
			"# @param Vector<Rational> points Optional. Should have length n-3-k. Gives the images of "
			"# the fixed vertices (besides the first one, which always goes to 0) as elements of R."
			"# If not given, all fixed vertices are mapped to 0"
			"# and the function computes the subdivision of M_0,n containing the recession fan of H_k(x)"
			"# @option Bool Verbose If true, the function outputs some progress information. True by default."
			"# @tparam Addition Min or Max, where the coordinates live."
			"# @return Cycle A subdivision of M_0,n",
			"hurwitz_subdivision<Addition>($,Vector<Int>;Vector<Rational> = new Vector<Rational>(),{Verbose=>1})");

	UserFunctionTemplate4perl("# @category Hurwitz cycles"
			"# This function computes the Hurwitz cycle H_k(x), x = (x_1,...,x_n)"
			"# @param Int k The dimension of the Hurwitz cycle, i.e. the number of moving vertices"
			"# @param Vector<Int> degree The degree x. Should add up to 0"
			"# @param Vector<Rational> points Optional. Should have length n-3-k. Gives the images of "
			"# the fixed vertices (besides 0). If not given all fixed vertices are mapped to 0"
			"# and the function computes the recession fan of H_k(x)"
			"# @option Bool Verbose If true, the function outputs some progress information. True by default."
			"# @tparam Addition Min or Max, where the coordinates live."
			"# @return Cycle<Addition> H_k(x), in homogeneous coordinates",
			"hurwitz_cycle<Addition>($,Vector<Int>;Vector<Rational> = new Vector<Rational>(),{Verbose=>1})");

	UserFunctionTemplate4perl("# @category Hurwitz cycles"
			"# This function computes hurwitz_subdivision and hurwitz_cycle at the same time, "
			"# returning the result in an array"
			"# @param Int k The dimension of the Hurwitz cycle, i.e. the number of moving vertices"
			"# @param Vector<Int> degree The degree x. Should add up to 0"
			"# @param Vector<Rational> points Optional. Should have length n-3-k. Gives the images of "
			"# the fixed vertices (besides 0). If not given all fixed vertices are mapped to 0"
			"# and the function computes the subdivision of M_0,n containing the recession fan of H_k(x)"
			"# @option Bool Verbose If true, the function outputs some progress information. True by default."
			"# @tparam Addition Min or Max, where the coordinates live."
			"# @return An array, containing first the subdivision of M_0,n, then the Hurwitz cycle",
			"hurwitz_pair<Addition>($,Vector<Int>;Vector<Rational> = new Vector<Rational>(),{Verbose=>1})");

	UserFunctionTemplate4perl("# @category Hurwitz cycles"
			"# Does the same as hurwitz_pair, except that no points are given and the user can give a "
			"# RationalCurve object representing a ray. If given, the computation"
			"# will be performed locally around the ray."
			"# @param Int k"
			"# @param Vector<Int> degree"
			"# @option Bool Verbose If true, the function outputs some progress information. True by default."
			"# @tparam Addition Min or Max, where the coordinates live."
			"# @param RationalCurve local_curve",
			"hurwitz_pair_local<Addition>($,Vector<Int>,RationalCurve,{Verbose=>1})");

	UserFunction4perl("# @category Abstract rational curves"
			"# Takes a RationalCurve and a list of node indices. Then inserts additional "
			"# leaves (starting from N_LEAVES+1) at these nodes and returns the resulting "
			"# RationalCurve object"
			"# @param RationalCurve curve A RationalCurve object"
			"# @param Vector<Int> nodes A list of node indices of the curve",
			&insert_leaves, "insert_leaves(RationalCurve,$)");

}}

