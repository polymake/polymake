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

	Contains a function to compute the cartesian product of cycles.
	*/

#ifndef POLYMAKE_ATINT_CARTESIAN_PRODUCT_H
#define POLYMAKE_ATINT_CARTESIAN_PRODUCT_H

#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/linalg.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/tropical/LoggingPrinter.h"
#include "polymake/tropical/thomog.h"
#include "polymake/tropical/misc_tools.h"
#include "polymake/tropical/specialcycles.h"

namespace polymake { namespace tropical {

	/**
	  @brief Takes a list of Cycle objects (that may be weighted but need not be) and computes the cartesian product of these. 
	  If any complex has weights, all non-weighted complexes will be treated as having constant weight 1. 
	  The [[LOCAL_RESTRICTION]] of the result will be the cartesian product of the [[LOCAL_RESTRICTION]]s of each complex. 
	  (If a complex does not have any restrictions, the new local restriction is the (pairwise) product of all 
	  local restriction cones with ALL cones (including faces) of the next complex)
	  @param Array<perl::Object> complexes A list of Cycle objects
	  @return Cycle The cartesian product of the complexes. Note that the representation is noncanonical, as it identifies
	  the product of two projective tori of dimensions d and e with a projective torus of dimension d+e
	  by dehomogenizing and then later rehomogenizing after the first coordinate.
	  */
	template <typename Addition>
		perl::Object cartesian_product(const Array<perl::Object> &complexes) {
			//dbgtrace << "Generating container variables for result" << endl;

			//** EXTRACT FIRST COMPLEX ********************************************

			perl::Object firstComplex = complexes[0];
			//This will contain the sets describing the maximal cones
			IncidenceMatrix<> maximalCones = firstComplex.give("MAXIMAL_POLYTOPES");
			//Will contain the rays
			Matrix<Rational> rayMatrix = firstComplex.give("VERTICES");
				rayMatrix = tdehomog(rayMatrix);
			//Will contain the lineality space
			Matrix<Rational> linMatrix = firstComplex.give("LINEALITY_SPACE");
				linMatrix = tdehomog(linMatrix);
			//Will contain the weights (if any)
			Vector<Integer> weights;
			bool product_has_weights = false;
			if(firstComplex.lookup("WEIGHTS") >> weights) {
				product_has_weights = true;
			}
			IncidenceMatrix<> local_restriction;
			if(firstComplex.exists("LOCAL_RESTRICTION")) {
				firstComplex.give("LOCAL_RESTRICTION") >> local_restriction;
			}
			bool product_has_lattice = firstComplex.exists("LATTICE_BASES");
			Matrix<Integer> product_l_generators;
			IncidenceMatrix<> product_l_bases;
			if(product_has_lattice) {
				//dbgtrace << "Extracting first lattice " << endl;
				Matrix<Integer> lg = firstComplex.give("LATTICE_GENERATORS");
				lg = tdehomog(lg);
				product_l_generators = lg;
				IncidenceMatrix<> product_l_bases = firstComplex.give("LATTICE_BASES");
			}

			//int product_dim = rayMatrix.cols() > linMatrix.cols() ? rayMatrix.cols() : linMatrix.cols();
			int product_dim = rayMatrix.rows() > 0? rayMatrix.cols() : linMatrix.cols();
			//Sort rays by affine and directional
			std::pair<Set<int>, Set<int> > product_vertex_pair = far_and_nonfar_vertices(rayMatrix);
			Set<int> product_affine = product_vertex_pair.second;
			Set<int> product_directional = product_vertex_pair.first;
			//dbgtrace << "Product affine " << product_affine << endl;
			//dbgtrace << "Product directional " << product_directional << endl;


			//dbgtrace << "Iterating over all " << complexes.size() -1 << " complexes: " << endl;

			//** ITERATE OTHER COMPLEXES ********************************************

			for(unsigned int i = 1; i < complexes.size(); i++) {
				//dbgtrace << "Considering complex nr. " << i+1 << endl;
				//Extract properties
				
				if(CallPolymakeFunction("is_empty",complexes[i])) {
					int projective_amb = std::max(rayMatrix.cols(), linMatrix.cols()) -1;
					for(int j = i; j < complexes.size(); j++) {
						int jth_projective_amb = complexes[j].give("PROJECTIVE_AMBIENT_DIM");
						projective_amb += jth_projective_amb;
					}
					return empty_cycle<Addition>(projective_amb);
				}

				bool uses_weights = false;
				Matrix<Rational> prerays = complexes[i].give("VERTICES");
					prerays = tdehomog(prerays);
				Matrix<Rational> prelin = complexes[i].give("LINEALITY_SPACE");
					prelin = tdehomog(prelin);
				IncidenceMatrix<> premax = complexes[i].give("MAXIMAL_POLYTOPES");
				IncidenceMatrix<> pre_local_restriction;
				if(complexes[i].exists("LOCAL_RESTRICTION")) {
						complexes[i].give("LOCAL_RESTRICTION") >> pre_local_restriction;
				}

				Array<Integer> preweights;
				if(complexes[i].exists("WEIGHTS")) {
					preweights = complexes[i].give("WEIGHTS");
					uses_weights = true; 
				}

				// ** RECOMPUTE RAY DATA ***********************************************

				//Sort rays
				std::pair<Set<int>, Set<int> > complex_vertex_pair = far_and_nonfar_vertices(prerays);
				Set<int> complex_affine = complex_vertex_pair.second;
				Set<int> complex_directional = complex_vertex_pair.first;
				//dbgtrace << "Affine: " << complex_affine << endl;
				//dbgtrace << "Directional: " << complex_directional << endl;
				//If this fan uses homog. coordinates, strip away the first column of rays and linear space
				if(prerays.rows() > 0) prerays = prerays.minor(All,~scalar2set(0));
				if(prelin.rows() > 0) prelin = prelin.minor(All,~scalar2set(0));
				//dbgtrace << "prerays: " << prerays << endl;
				//dbgtrace << "rcols: " << prerays.cols() << endl;
				//dbgtrace << "lcols: " << prelin.cols() << endl;
				//int dim = prerays.cols() > prelin.cols() ? prerays.cols() : prelin.cols();
				int dim = prerays.rows() > 0? prerays.cols() : prelin.cols();
				//dbgtrace << "dim: " << dim << endl;

				//dbgtrace << "Creating ray matrix" << endl;
				// 	dbgtrace << "Affine rays of product are " << rayMatrix.minor(product_affine,All) << endl;
				// 	dbgtrace << "Affine rays of complex are " << prerays.minor(complex_affine,All) << endl;

				//Create new ray matrix
				Matrix<Rational> newRays(0,product_dim + dim);
				//First create affine rays
				// 	Map<std::pair<int,int>,int> affineIndices; //For index conversion
				Map<int, Map<int,int> > affineIndices;
				for(Entire<Set<int> >::iterator prays = entire(product_affine); !prays.at_end(); prays++)  {
					affineIndices[*prays] = Map<int,int>();
					Vector<Rational> pRay;
					if(*prays >= 0) pRay = rayMatrix.row(*prays);
					else pRay = zero_vector<Rational>(product_dim);
					for(Entire<Set<int> >::iterator crays = entire(complex_affine); !crays.at_end(); crays++) {
						Vector<Rational> cRay;
						if(*crays >= 0) cRay = prerays.row(*crays);
						else cRay = zero_vector<Rational>(dim);
						newRays = newRays / (pRay | cRay);
						affineIndices[*prays][*crays] = newRays.rows()-1;
						// 		affineIndices(*prays,*crays) = newRays.rows()-1;
					}
				}
				Set<int> newAffine = sequence(0, newRays.rows());

				//dbgtrace << "New affine rays read " << newRays << endl;

				//dbgtrace << "Adding directional rays" << endl;
				//Now add the directional rays of both cones
				Map<int,int> pdirIndices;
				Map<int,int> cdirIndices; //For index conversion
				Vector<Rational> product_zero = zero_vector<Rational>(product_dim);
				Vector<Rational> complex_zero = zero_vector<Rational>(dim);
				for(Entire<Set<int> >::iterator prays = entire(product_directional); !prays.at_end(); prays++)  {
					newRays = newRays / (rayMatrix.row(*prays) | complex_zero);
					pdirIndices[*prays] = newRays.rows()-1;
				}
				for(Entire<Set<int> >::iterator crays = entire(complex_directional); !crays.at_end(); crays++) {
					newRays = newRays / (product_zero | prerays.row(*crays));
					cdirIndices[*crays] = newRays.rows()-1;
				}
				Set<int> newDirectional = sequence(newAffine.size(),product_directional.size() + complex_directional.size());

				//dbgtrace << "Creating lineality matrix" << endl;

				//Create new lineality matrix
				if(prelin.rows() > 0) {
					prelin = Matrix<Rational>(prelin.rows(),product_dim) | prelin;
				}
				if(linMatrix.rows() > 0) {
					linMatrix = linMatrix | Matrix<Rational>(linMatrix.rows(), dim);
				}

				//dbgtrace << "Prelin = " << prelin << "\nlinMatrix = " << linMatrix << endl;

				// ** RECOMPUTE LATTICE DATA ***************************************

				//Compute lattice data
				bool complex_has_lattice = complexes[i].exists("LATTICE_BASES");
				product_has_lattice = product_has_lattice && complex_has_lattice;
				int lattice_index_translation = 0; //Number of row where new lattice gens. begin

				IncidenceMatrix<> new_lattice_bases;

				Matrix<Integer> complex_lg;
				IncidenceMatrix<> complex_lb;
				if(product_has_lattice) {
					Matrix<Integer> clg = complexes[i].give("LATTICE_GENERATORS");
						clg = tdehomog(clg);
					complex_lg = clg;
					IncidenceMatrix<> clb = complexes[i].give("LATTICE_BASES");
					complex_lb = clb;
					//Compute cartesian product of lattice matrices:
					//Adjust dimension, then concatenate
					if(complex_lg.rows() > 0) complex_lg = complex_lg.minor(All,~scalar2set(0));
					product_l_generators = 
						product_l_generators | Matrix<Integer>(product_l_generators.rows(), dim);
					complex_lg = 
						Matrix<Integer>(complex_lg.rows(), product_dim) | complex_lg;	    
					lattice_index_translation = product_l_generators.rows();
					product_l_generators /= complex_lg;

				}

				// ** RECOMPUTE CONES *******************************************



				//dbgtrace << "Creating cones" << endl;

				//dbgtrace << "Ray matrix is " << newRays << endl;
				//dbgtrace << "Affine indices " << affineIndices << endl;
				//dbgtrace << "Directional indices product" << pdirIndices << endl;
				//dbgtrace << "Directional indices complex" << cdirIndices << endl;

				//Now create the new cones and weights:
				IncidenceMatrix<> newMaxCones(0,newRays.rows());
				Vector<Integer> newWeights;
				//Make sure, we have at least one "cone" in each fan, even if it is empty
				if(premax.rows() == 0) { premax = premax / Set<int>();}
				for(int pmax = 0; pmax < maximalCones.rows(); pmax++) {
					Set<int> product_cone = maximalCones.row(pmax);
					//dbgtrace << "Product cone: " << product_cone << endl;
					for(int cmax = 0; cmax < premax.rows(); cmax++) {
						Set<int> complex_cone = premax.row(cmax);
						//dbgtrace << "Complex cone: " << complex_cone << endl;
						Set<int> newcone;
						Set<int> pAffine = product_cone * product_affine;
						Set<int> pDirectional = product_cone * product_directional;
						Set<int> cAffine = complex_cone * complex_affine;
						Set<int> cDirectional = complex_cone * complex_directional;

						//First add the affine rays: For each pair of affine rays add the corresponding index from
						//affineIndices
						for(Entire<Set<int> >::iterator pa = entire(pAffine); !pa.at_end(); pa++) {
							for(Entire<Set<int> >::iterator ca = entire(cAffine); !ca.at_end(); ca++) {
								// 		    if(cmax + pmax == 0) dbgtrace << *pa << "," << *ca << ": " << affineIndices[std::make_pair(*pa,*ca)] << endl;
								newcone = newcone + affineIndices[*pa][*ca];
							}		
						}
						//Now add the directional indices
						for(Entire<Set<int> >::iterator pd = entire(pDirectional); !pd.at_end(); pd++) {
							newcone = newcone + pdirIndices[*pd];
						}
						for(Entire<Set<int> >::iterator cd = entire(cDirectional); !cd.at_end(); cd++) {
							newcone = newcone + cdirIndices[*cd];
						}
						//dbgtrace << "Result: " << newcone << endl;
						newMaxCones /= newcone;
						//Compute weight
						if(product_has_weights || uses_weights) {
							newWeights = newWeights | (product_has_weights? weights[pmax] : Integer(1)) * (uses_weights? preweights[cmax] : Integer(1));
						}
						//Compute lattice data
						if(product_has_lattice) {
							Set<int> cone_l_basis = product_l_bases.row(pmax) + Set<int>( translate(complex_lb.row(cmax),lattice_index_translation));
							new_lattice_bases /= cone_l_basis;
						}
					}
				}

				//dbgtrace << "Maximal cones now: " << newMaxCones << endl;

				//Compute the cross product of the local_restrictions
				IncidenceMatrix<> new_local_restriction(0,newRays.rows());
				if(local_restriction.rows() > 0 || pre_local_restriction.rows() > 0) {
					//If one variety is not local, we take all its cones for the product
					IncidenceMatrix<> product_locality(local_restriction);
					if(product_locality.rows() == 0) {
						perl::Object current_product("fan::PolyhedralComplex");
							current_product.take("VERTICES") << rayMatrix;
							current_product.take("MAXIMAL_POLYTOPES") << maximalCones;
						product_locality = all_cones_as_incidence(current_product); 
					}
					IncidenceMatrix<> pre_locality(pre_local_restriction);
					if(pre_locality.rows() == 0) {
						pre_locality = all_cones_as_incidence(complexes[i]); 
					}

					//dbgtrace << "pro_locality " << product_locality << endl;
					//dbgtrace << "pre_locality " << pre_locality << endl;

					for(int i = 0; i < product_locality.rows(); i++) {
						Set<int> pAffine = product_locality.row(i) * product_affine;
						Set<int> pDirectional = product_locality.row(i) * product_directional;
						for(int j = 0; j < pre_locality.rows(); j++) {
							Set<int> local_cone;
							Set<int> cAffine = pre_locality.row(j) * complex_affine;
							Set<int> cDirectional = pre_locality.row(j) * complex_directional;
							//First add the affine rays: For each pair of affine rays add the corresponding index from
							//affineIndices
							for(Entire<Set<int> >::iterator pa = entire(pAffine); !pa.at_end(); pa++) {
								for(Entire<Set<int> >::iterator ca = entire(cAffine); !ca.at_end(); ca++) {
									local_cone = local_cone + affineIndices[*pa][*ca];
								}		
							}
							//Now add the directional indices
							for(Entire<Set<int> >::iterator pd = entire(pDirectional); !pd.at_end(); pd++) {
								local_cone = local_cone + pdirIndices[*pd];
							}
							for(Entire<Set<int> >::iterator cd = entire(cDirectional); !cd.at_end(); cd++) {
								local_cone = local_cone + cdirIndices[*cd];
							}
							new_local_restriction /=  local_cone;
						}
					}
				}

				// ** COPY VALUES ONTO NEW PRODUCT ***************************************

				//Copy values
				rayMatrix = newRays;
				linMatrix = linMatrix.rows() == 0? prelin : (prelin.rows() == 0? linMatrix : linMatrix / prelin);
				product_dim = rayMatrix.cols() > linMatrix.cols() ? rayMatrix.cols() : linMatrix.cols();
				product_affine = newAffine;
				product_directional = newDirectional;
				maximalCones = newMaxCones;
				weights = newWeights;
				product_has_weights = product_has_weights ||  uses_weights;
				local_restriction = new_local_restriction;
				product_l_bases = new_lattice_bases;
			}

			//Fill fan with result
			perl::Object result(perl::ObjectType::construct<Addition>("Cycle"));
			result.take("VERTICES") << thomog(rayMatrix);
			result.take("LINEALITY_SPACE") << thomog(linMatrix);
			result.take("MAXIMAL_POLYTOPES") << maximalCones;
			if(product_has_weights) result.take("WEIGHTS") << weights;
			if(local_restriction.rows() > 0)
				result.take("LOCAL_RESTRICTION") << local_restriction;
			if(product_has_lattice) {
				result.take("LATTICE_BASES") << product_l_bases;
				result.take("LATTICE_GENERATORS") << thomog(product_l_generators);
			}

			return result;

		}
}}

#endif
