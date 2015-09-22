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

	Implements the composition of two morphisms.
	*/

#ifndef POLYMAKE_ATINT_MORPHISM_COMPOSITION_H
#define POLYMAKE_ATINT_MORPHISM_COMPOSITION_H

#include "polymake/client.h"
#include "polymake/Matrix.h"
#include "polymake/Rational.h"
#include "polymake/Vector.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/tropical/LoggingPrinter.h"
#include "polymake/tropical/thomog.h"
#include "polymake/tropical/morphism_thomog.h"
#include "polymake/tropical/refine.h"
#include "polymake/tropical/morphism_values.h"
#include "polymake/tropical/solver_def.h"

namespace polymake { namespace tropical {

	using namespace atintlog::donotlog;
	//using namespace atintlog::dolog;
	//using namespace atintlog::dotrace;

	/**
	  @brief Computes the composition g(f) of two morphisms f and g (as in f:X->Y, g:Y->Z). 
	  Actually, f and g can also be piecewise linear maps on their domains, the method will work equally well. 
	  The function does not require that g's [[DOMAIN]] contain the image of f, the composition will always
	  be defined on the preimage of g's [[DOMAIN]] under f.
	  @param perl::Object f A Morphism
	  @param perl::Object g A Morphism 
	  @return perl::Object A Morphism object, the composition "g after f" The weights of f's domain are
	  copied onto the cones of the new domain, where they have full dimension.
	  */
	template <typename Addition>
		perl::Object morphism_composition(perl::Object f, perl::Object g) {

			// -------------------------- PREPARATIONS ----------------------------------- //

			//Extract values of f
			perl::Object f_domain = f.give("DOMAIN");
			Matrix<Rational> f_rays = f_domain.give("SEPARATED_VERTICES");
			Matrix<Rational> f_lin = f_domain.give("LINEALITY_SPACE");
			bool f_has_weights = f_domain.exists("WEIGHTS");
			Vector<Integer> f_domain_weights;
			if(f_has_weights)
				f_domain.give("WEIGHTS") >> f_domain_weights;
			IncidenceMatrix<> f_cones = f_domain.give("SEPARATED_MAXIMAL_POLYTOPES");
			bool f_has_matrix = f.exists("MATRIX") || f.exists("TRANSLATE");
			Matrix<Rational> f_on_rays = f.give("VERTEX_VALUES");
			Matrix<Rational> f_on_lin = f.give("LINEALITY_VALUES");
			Matrix<Rational> f_prop_matrix; Vector<Rational> f_prop_translate;
			Matrix<Rational> f_dehomog_matrix; Vector<Rational> f_dehomog_translate;
			if(f_has_matrix) {
				f.give("MATRIX") >> f_prop_matrix;
				f.give("TRANSLATE") >> f_prop_translate;
				std::pair<Matrix<Rational>, Vector<Rational> > dehom_f = tdehomog_morphism(f_prop_matrix, f_prop_translate);
				f_dehomog_matrix = dehom_f.first;
				f_dehomog_translate = dehom_f.second;
			}
			Vector< Matrix<Rational> > f_hreps_ineq;
			Vector< Matrix<Rational> > f_hreps_eq;

			//Now tropically dehomogenize everything and create version of f-values with leading coordinate.
			f_rays = tdehomog(f_rays,0);
			f_lin = tdehomog(f_lin,0);
			f_on_rays = tdehomog(f_on_rays,0,false);
			f_on_lin = tdehomog(f_on_lin,0,false);
			Matrix<Rational> f_on_rays_homog = f_rays.col(0) | f_on_rays;
			Matrix<Rational> f_on_lin_homog = zero_vector<Rational>() | f_on_lin;

			int f_ambient_dim = std::max(f_rays.cols(), f_lin.cols()); 

			//Extract values of g
			perl::Object g_domain = g.give("DOMAIN");
			bool g_has_matrix = g.exists("MATRIX") || g.exists("TRANSLATE");
			Matrix<Rational> g_prop_matrix; Vector<Rational> g_prop_translate;
			Matrix<Rational> g_dehomog_matrix; Vector<Rational> g_dehomog_translate;
			if(g_has_matrix) {
				g.give("MATRIX") >> g_prop_matrix;
				g.give("TRANSLATE") >> g_prop_translate;
				std::pair<Matrix<Rational>, Vector<Rational> > dehom_g = tdehomog_morphism(g_prop_matrix, g_prop_translate);
				g_dehomog_matrix = dehom_g.first;
				g_dehomog_translate = dehom_g.second;
			}
			Matrix<Rational> g_rays = g_domain.give("SEPARATED_VERTICES");
			Matrix<Rational> g_lin = g_domain.give("LINEALITY_SPACE");
			int g_domain_dim = g_domain.give("PROJECTIVE_DIM");
			IncidenceMatrix<> g_cones = g_domain.give("SEPARATED_MAXIMAL_POLYTOPES");
			Matrix<Rational> g_on_rays = g.give("VERTEX_VALUES");
			Matrix<Rational> g_on_lin = g.give("LINEALITY_VALUES");
			Vector< Matrix<Rational> > g_hreps_ineq;
			Vector< Matrix<Rational> > g_hreps_eq;

			//Tropically dehomogenize everything - g's values don't need a leading coordinate
			g_rays = tdehomog(g_rays,0);
			g_lin = tdehomog(g_lin,0);
			g_on_rays = tdehomog(g_on_rays,0,false);
			g_on_lin = tdehomog( g_on_lin,0,false);	

			//Prepare result variables
			Matrix<Rational> pullback_rays(0, f_ambient_dim);
			Matrix<Rational> pullback_lineality(0, f_ambient_dim);
			bool lineality_computed = false;
			Vector<Set<int> > pullback_cones;
			Set<Set<int> > pullback_cones_set; //Used to check for doubles
			Matrix<Rational> pullback_ray_values(0,g_on_rays.cols());
			Matrix<Rational> pullback_lin_values(0,g_on_lin.cols());
			Vector<Integer> pullback_weights;
			//The following two variables contain for each cone of the pullback domain the representation 
			//as an affine linear function on this cone
			Vector<Matrix<Rational> > pullback_matrices; 
			Vector<Vector<Rational> > pullback_translates;

			//Compute H-representations of all cones in f_domain and g_domain
			solver<Rational> sv;
			for(int fcone = 0; fcone < f_cones.rows(); fcone++) {
				std::pair<Matrix<Rational>, Matrix<Rational> > p = sv.enumerate_facets(
						f_rays.minor(f_cones.row(fcone),All), 
						f_lin, false,false);
				f_hreps_ineq |= p.first;
				f_hreps_eq |= p.second;
			}//END compute fcone-H-rep
			for(int gcone = 0; gcone < g_cones.rows(); gcone++) {
				std::pair<Matrix<Rational>, Matrix<Rational> > p = sv.enumerate_facets(
						g_rays.minor(g_cones.row(gcone),All), 
						g_lin, false,false);
				g_hreps_ineq |= p.first;
				g_hreps_eq |= p.second;
			}//END compute gcone-H-rep


			bool have_full_dimensional_pullback_cone = false;

			// --------------------------- COMPUTE GEOMETRY ---------------------------------- //

			//Now iterate all cones of f's domain
			for(int fcone = 0; fcone < f_cones.rows(); fcone++) {
				//dbgtrace << "Computing on function cone " << fcone << endl;
				//Compute H-representation of the image of the cone
				std::pair<Matrix<Rational>, Matrix<Rational> > image_rep = sv.enumerate_facets(
						(f_on_rays_homog.minor(f_cones.row(fcone),All)),
						f_on_lin_homog, false, false); 

				int image_dim = image_rep.first.cols() - image_rep.second.rows() - 1;

				dbgtrace << "Image of cone has H-rep " << image_rep << endl;

				//Compute representation of morphism on current cone
				Matrix<Rational> fmatrix;
				Vector<Rational> ftranslate;
				if(f_has_matrix) {
					fmatrix = f_dehomog_matrix;
					ftranslate = f_dehomog_translate;
				}
				else {
					computeConeFunction(f_rays.minor(f_cones.row(fcone),All), f_lin,
							f_on_rays.minor(f_cones.row(fcone),All),
							f_on_lin, ftranslate, fmatrix);
				}

				dbgtrace << "Local representation is " << ftranslate << " and " << fmatrix << endl;

				//Iterate all cones of the function
				for(int gcone = 0; gcone < g_cones.rows(); gcone++) {
					dbgtrace << "Intersecting with g cone " << gcone << endl;
					//Compute intersection 
					Matrix<Rational> intersection_ineq;
					Matrix<Rational> intersection_eq;

					//Compute an irredundant H-rep of the intersection
					dbgtrace << "Computing irredundant H-rep" << endl;
					dbgtrace << image_rep.first << "\n" << g_hreps_ineq[gcone] << endl;

					intersection_ineq = (image_rep.first) / (g_hreps_ineq[gcone]);
					intersection_eq = (image_rep.second) / (g_hreps_eq[gcone]);
					Matrix<Rational> isMatrix = intersection_ineq / intersection_eq;

					dbgtrace << "Canonicalizing..." << endl;

					std::pair<Bitset,Bitset> isection = 
						sv.canonicalize( intersection_ineq,intersection_eq,1);

					dbgtrace << "Assigning canonical rays " << endl;

					intersection_ineq = isMatrix.minor(isection.first,All);
					intersection_eq = isMatrix.minor(isection.second,All);


					int interdim = isMatrix.cols()  - isection.second.size() - 1 ;
					dbgtrace << "intersection dimension is " << interdim << endl;

					//Check dimension of intersection - if its not the correct one, take the next g cone
					if( interdim != std::min(image_dim, g_domain_dim)) continue;

					dbgtrace << "Cone is valid " << endl;	    

					dbgtrace << "Intersection cone has H-rep " << intersection_ineq << "\n" << intersection_eq << endl;

					dbgtrace << "Computing representation on g cone" << endl;

					//Compute g's representation on the current cone
					Vector<Rational> gtranslate;
					Matrix<Rational> gmatrix;
					if(g_has_matrix) {
						gmatrix = g_dehomog_matrix;
						gtranslate = g_dehomog_translate;
					}
					else {
						dbgtrace << g_cones.row(gcone) << endl;
						dbgtrace << "Rays of g: " << g_rays.minor(g_cones.row(gcone),All) << "," << g_lin << endl;
						dbgtrace << g_on_rays << endl;
						dbgtrace << "Values " << g_on_rays.minor(g_cones.row(gcone),All) << "," << g_on_lin << endl;
						computeConeFunction(g_rays.minor(g_cones.row(gcone),All), g_lin, 
								g_on_rays.minor(g_cones.row(gcone),All), g_on_lin, gtranslate, 
								gmatrix);
					}

					dbgtrace << "g's representation on this cone " << gmatrix << " and " << gtranslate << endl;

					//Compute preimage of the intersection cone
					//If (b,-A) is the representation of (in)equalities of the cone
					// and x |-> v + Mx is the representation of the morphism, then
					//(b - Av, -AM) is the representation of the preimage

					dbgtrace << "Computing preimage and pullback" << endl;

					Matrix<Rational> preimage_ineq = intersection_ineq.minor(All,~scalar2set(0)) * fmatrix;
					preimage_ineq = (intersection_ineq.col(0) + intersection_ineq.minor(All,~scalar2set(0)) * ftranslate)
						| preimage_ineq;

					Matrix<Rational> preimage_eq(0,preimage_ineq.cols());
					if(intersection_eq.rows() > 0) { //For the equalities consider the special case that there are none
						preimage_eq = intersection_eq.minor(All,~scalar2set(0)) * fmatrix;
						preimage_eq =  
							((intersection_eq.col(0) + intersection_eq.minor(All,~scalar2set(0)) * ftranslate) 
							 | preimage_eq);
					}

					//Intersect with the fcone
					preimage_ineq /= f_hreps_ineq[fcone];
					preimage_eq /= f_hreps_eq[fcone];


					//dbgtrace << "Preimage ineq " << preimage_ineq << "\n eq " << preimage_eq << endl;


					std::pair<Matrix<Rational>, Matrix<Rational> > preimage_cone;
					try {
						preimage_cone = sv.enumerate_vertices(
								preimage_ineq, preimage_eq, false,true);
					}
					catch(...) {
						preimage_cone = std::make_pair(Matrix<Rational>(0,f_ambient_dim), Matrix<Rational>(0,f_ambient_dim));
					}

					dbgtrace << "Preimage has rays " << preimage_cone.first << " and lin " << preimage_cone.second << endl;

					//dbgtrace << "Canonicalizing rays" << endl;

					Matrix<Rational> preimage_rays = preimage_cone.first;
					Matrix<Rational> preimage_lin = preimage_cone.second;

					//Canonicalize rays and create cone
					if(!lineality_computed) {
						pullback_lineality = preimage_lin;
						lineality_computed = true;
						dbgtrace << "Setting lineality to " << pullback_lineality << endl;
					}
					Set<int> pcone; 
					for(int r = 0; r < preimage_rays.rows(); r++) {
						//Canonicalize ray
						if(preimage_rays(r,0) != 0) {
							preimage_rays.row(r) *= (1/preimage_rays(r,0));
						}
						else {
							for(int c = 1; c < preimage_rays.cols(); c++) {
								if(preimage_rays(r,c) != 0) {
									preimage_rays.row(r) *= (1/abs(preimage_rays(r,c)));
									break;
								}
							}
						}
						//Find correct ray index
						int ray_index = -1;
						for(int oray = 0; oray < pullback_rays.rows(); oray++) {
							if(pullback_rays.row(oray) == preimage_rays.row(r)) {
								ray_index = oray; 
								break;
							}
						}
						//Insert ray if necessary and add index to set
						if(ray_index == -1) {
							pullback_rays /= preimage_rays.row(r);
							ray_index = pullback_rays.rows() -1;
						}
						pcone += ray_index;
					}
					dbgtrace << "Ray set is " << pcone << endl;
					//Add cone if it doesn't exist yet
					if(!pullback_cones_set.contains(pcone)) {
						pullback_cones |= pcone;
						pullback_cones_set += pcone;
						dbgtrace << "Adding cone " << pcone << endl;

						if(interdim == image_dim)
							have_full_dimensional_pullback_cone = true;
						//If the pullback cone has full dimension (<=> the dimension of
						//the intersection is the dimension of the image cone), we copy f's domain's weight.
						if(f_has_weights){ 
							pullback_weights |= (interdim == image_dim? f_domain_weights[fcone] :0);
						}

						//Now we compute the representation of h = g after f 
						Matrix<Rational> hmatrix = gmatrix * fmatrix;
						Vector<Rational> htranslate = gmatrix * ftranslate + gtranslate;

						dbgtrace << "Composition on preimage: " << hmatrix << " and " << htranslate << endl;

						pullback_matrices |= hmatrix;
						pullback_translates |= htranslate;

					}
					//dbgtrace << "Rays now read " << pullback_rays << endl;


				}//END iterate all cones of morphism g 

			}//END iterate cones of morphism f

			// ------------------------- COMPUTE VALUES --------------------------------- //

			//dbgtrace << "Computing values on SEPARATED_VERTICES " << endl;

			//Compute SEPARATED_VERTICES / SEPARATED_MAXIMAL_POLYTOPES

			perl::Object pullback_domain(perl::ObjectType::construct<Addition>("Cycle"));
			pullback_domain.take("VERTICES") << thomog(pullback_rays);
			pullback_domain.take("MAXIMAL_POLYTOPES") << pullback_cones;
			pullback_domain.take("LINEALITY_SPACE") << thomog(pullback_lineality);
			if( (f_has_weights && have_full_dimensional_pullback_cone) || pullback_rays.rows() == 0) { 
				pullback_domain.take("WEIGHTS") << pullback_weights;
			}
			Matrix<Rational> pb_cmplx_rays = pullback_domain.give("SEPARATED_VERTICES");
			Matrix<Rational> pb_crays_dehomog = (tdehomog(pb_cmplx_rays)).minor(All,~scalar2set(0));
			IncidenceMatrix<> pb_cmplx_cones = pullback_domain.give("SEPARATED_MAXIMAL_POLYTOPES");
			IncidenceMatrix<> pb_cones_by_rays = T(pb_cmplx_cones);

			dbgtrace << "Pullback rays: " << pb_cmplx_rays << endl;
			dbgtrace << "Pullback cones: " << pb_cmplx_cones << endl;
			

			//Go trough all rays
			int basepoint = -1; //Save the first vertex
			for(int cr = 0; cr < pb_cmplx_rays.rows(); cr++) {
				dbgtrace << "Value on ray " << pb_cmplx_rays.row(cr) << endl;
				//Take any cone containing this ray
				int cone_index = *(pb_cones_by_rays.row(cr).begin());
				Matrix<Rational> cone_matrix = pullback_matrices[cone_index];
				Vector<Rational> cone_translate = pullback_translates[cone_index];
				dbgtrace << "Matrix is " <<  cone_matrix << cone_translate << endl;
				//If its a vertex, just compute the value
				if(pb_cmplx_rays(cr,0) == 1) {
					pullback_ray_values /= 
						(cone_matrix* pb_crays_dehomog.row(cr) + cone_translate);
					if(basepoint == -1) basepoint = cr;
				}
				//Otherwise find an associated vertex
				else {
					Set<int> rays_in_cone = pb_cmplx_cones.row(cone_index);
					for(Entire<Set<int> >::iterator aRay = entire(rays_in_cone); !aRay.at_end(); aRay++) {
						if(pb_cmplx_rays(*aRay,0) == 1) {
							Vector<Rational> sum = pb_crays_dehomog.row(*aRay) + pb_crays_dehomog.row(cr);
							pullback_ray_values /= 
								(
								 (cone_matrix* sum + cone_translate) - 
								 (cone_matrix* pb_crays_dehomog.row(*aRay) + cone_translate)
								);
							break;
						}
					}
				}
			}

			//Now compute lineality values
			Matrix<Rational> pb_lin_dehomog = pullback_lineality.minor(All,~scalar2set(0));
			for(int l = 0; l < pullback_lineality.rows(); l++) {
				Vector<Rational> sum = pb_crays_dehomog.row(basepoint) + pb_lin_dehomog.row(l);
				pullback_lin_values /= 
					(
					 (pullback_matrices[0] * sum + pullback_translates[0]) - 
					 pullback_ray_values.row(basepoint)
					);
			}

			// ------------------------------- RETURN RESULT ------------------------------- //

			dbgtrace << "Done. Preparing result" << endl;

			perl::Object result(perl::ObjectType::construct<Addition>("Morphism"));
			result.take("DOMAIN") << pullback_domain;
			result.take("VERTEX_VALUES") << thomog(pullback_ray_values,0,false);
			result.take("LINEALITY_VALUES") << thomog(pullback_lin_values,0,false);

			if(f_has_matrix && g_has_matrix) {
				result.take("MATRIX") << (g_prop_matrix * f_prop_matrix);
				result.take("TRANSLATE") << (g_prop_matrix * f_prop_translate + g_prop_translate);
			}

			return result;

		}//END morphism_composition
}}

#endif
