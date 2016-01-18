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

	Contains functionality for computing divisors
	*/

#ifndef POLYMAKE_ATINT_DIVISOR_H
#define POLYMAKE_ATINT_DIVISOR_H

#include "polymake/client.h"
#include "polymake/Set.h"
#include "polymake/Map.h"
#include "polymake/Array.h"
#include "polymake/Matrix.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Vector.h"
#include "polymake/Rational.h"
#include "polymake/tropical/thomog.h"
#include "polymake/tropical/minimal_interior.h"
#include "polymake/tropical/specialcycles.h"
#include "polymake/tropical/LoggingPrinter.h"


namespace polymake { namespace tropical {

	typedef Map<std::pair<int,int>, Vector<Integer> > LatticeMap ;
	typedef Map<std::pair<int,int>, Vector<Rational> > LatticeFunctionMap;

	//using namespace atintlog::donotlog;
	//using namespace atintlog::dolog;
	//using namespace atintlog::dotrace;

	/**
	  @brief Takes as input a tropical cycle and a matrix of rational values. 
	  Each row of the matrix is interpreted as a value vector on the [[SEPARATED_VERTICES]] and 
	  [[LINEALITY_SPACE]] generators. The row count of the matrix is arbitrary in principle, 
	  but should be smaller than or equal to the dimension of the cycle. The function will then compute 
	  the Weil divisor obtained by intersecting with all the functions described by the rows (starting from top). 
	  Note that this still produces a meaningful result, if the Cycle is not balanced: 
	  The "divisor" of a given function is computed by taking all codim-1-faces, at which f is balanced and 
	  computing weights there.
	  @param Cycle fan A tropical variety
	  @param Matrix<Rational> values A matrix of rational values
	  @tparam Addition Whether divisor values are computed using min or max.
	  @return The divisor r_k * ... * r_1 * fan, where r_i is the function described by the i-th row.
	  */
	template <typename Addition>
		perl::Object divisorByValueMatrix(perl::Object complex, Matrix<Rational> values) {
			//This value carries all the intermediate results.
			perl::Object result = complex;      

			//Now we extract the values that we will later recompute by hand or that don't change at all

			Matrix<Rational> rays = complex.give("VERTICES");
			Matrix<Rational> crays = complex.give("SEPARATED_VERTICES");
			Vector<Integer> weights = complex.give("WEIGHTS");
			Matrix<Rational> lineality_space = complex.give("LINEALITY_SPACE");
			int lineality_dim = complex.give("LINEALITY_DIM");
			IncidenceMatrix<> local_restriction;
			if(complex.exists("LOCAL_RESTRICTION")) {
					 complex.give("LOCAL_RESTRICTION") >> local_restriction;
			}

			Matrix<Integer> lattice_generators = complex.give("LATTICE_GENERATORS");
			IncidenceMatrix<> lattice_bases = complex.give("LATTICE_BASES");

			//dbgtrace << "Rays: " << crays << endl;
			//dbgtrace << "Values: " << values << endl;

			//Do a compatibility check on the value matrix to avoid segfaults in the case of faulty input
			if(values.cols() != crays.rows() + lineality_space.rows()) {
				throw std::runtime_error("Value matrix is not compatible with variety. Aborting computation");
			}

			Matrix<Rational> lineality_values = values.minor(All,~(sequence(0,values.cols() - lineality_dim)));

			//Prepare the additional variables that will be used in all but the first iteration to recompute the
			//values vector

			//Contains at position i the row index of ray i in the ray matrix of the iteration before
			Vector<int> newRaysToOldRays; 

			//Contains the SEPARATED_MAXIMAL_POLYTOPES of the iteration before 
			IncidenceMatrix<> cmplx_oldcones;

			//Tells which new maximal cone is contained in which old maximal cone (this is essentially the
			//MAXIMAL_AT_CODIM_ONE without the rows for cones of weight 0)
			IncidenceMatrix<> newConesInOld;

			//For each cmplx_ray in the LAST iteration, this tells which should be the appropriate
			//column index in values for function value computation
			Vector<int> cmplx_origins (sequence(0,values.cols() - lineality_dim));

			//Contains the conversion vector for the last iteration (this one we recompute during
			//value recomputation)
			Vector<int> old_conversion;

			//Only uses in the fan case: For all iterations but the first it contains the set of rays of
			//the last iteration that remained after computing the divisor
			Set<int> remainingFanRays;

			//When computing the codim-one-weights, this contains the correct function value vector for the current iteration
			//When computing the new function vector for the current iteration, this means it contains the function
			//values of the old iteration
			Vector<Rational> currentValues;

			//Now we iterate through the matrix rows 
			for(int r = 0; r < values.rows(); r++) {
				//dbgtrace << "Computing on row " << r << endl;
				//First we recompute values that we can't/won't compute by hand

				IncidenceMatrix<> codimOneCones = result.give("CODIMENSION_ONE_POLYTOPES");
				if(codimOneCones.rows() == 0) 
					return empty_cycle<Addition>(std::max(rays.cols(),lineality_space.cols())-2); 
				IncidenceMatrix<> coneIncidences = result.give("MAXIMAL_AT_CODIM_ONE");

				LatticeMap latticeNormals = result.give("LATTICE_NORMALS");
				LatticeFunctionMap lnFunctionVector = result.give("LATTICE_NORMAL_FCT_VECTOR");
				Matrix<Rational> lsumFunctionVector = result.give("LATTICE_NORMAL_SUM_FCT_VECTOR");
				Vector<bool> balancedFaces = result.give("BALANCED_FACES");

				//dbgtrace << "Balanced faces: " << balancedFaces << endl;

				//Recompute the lattice bases
				Vector<Set<int> > new_lattice_bases;
				for(int co = 0; co < codimOneCones.rows(); co++) {
					new_lattice_bases |= lattice_bases.row(*(coneIncidences.row(co).begin()));
					//dbgtrace << "Co " << co << ": " << new_lattice_bases[new_lattice_bases.dim()-1] << " from " << *(coneIncidences.row(co).begin()) << 	  endl;
				}
				lattice_bases = new_lattice_bases;
				//dbgtrace << "lb: " << lattice_bases << endl;

				//Now we compute the correct value vector:

				if(r == 0) {
					currentValues = values.row(r);
				}
				else {
					currentValues = Vector<Rational>();
					Matrix<Rational> cmplx_rays = result.give("SEPARATED_VERTICES");
					Vector<int> conversion_vector = result.give("SEPARATED_CONVERSION_VECTOR");
					//Compute the maximal cones containing each cmplx_ray
					IncidenceMatrix<> cmplx_cones_t = result.give("SEPARATED_MAXIMAL_POLYTOPES");
					cmplx_cones_t = T(cmplx_cones_t);

					Vector<int> newcmplx_origins;  
					for(int cr = 0; cr < cmplx_rays.rows(); cr++) {
						//Find the corresponding cmplx_ray in the last iteration
						int mc = *(cmplx_cones_t.row(cr).begin()); //A cone containing the ray
						int oc = *(newConesInOld.row(mc).begin()); //An old cone containing mc
						//Now find the cmplx_ray of the old cone, such that 
						//its corresponding ray is equal to the corresponding ray of the new ray
						Set<int> ocrays = cmplx_oldcones.row(oc);
						for(Entire<Set<int> >::iterator ocr = entire(ocrays); !ocr.at_end(); ocr++) {
							//If the old ray (in non-complex counting in the old iteration) is the same as 
							//the new ray (in non-complex counting) in the new iteration, we can
							//copy its function column index
							if(old_conversion[*ocr] == newRaysToOldRays[conversion_vector[cr]]) {
								currentValues |= values(r,cmplx_origins[*ocr]);
								newcmplx_origins |= cmplx_origins[*ocr];
								break;
							}
						}
					}
					cmplx_origins = newcmplx_origins;
					//Finally append lineality values
					if(lineality_values.rows() > 0)
						currentValues |= lineality_values.row(r);
				}
				//dbgtrace << "Value vector is: " << currentValues << endl;

				//Then we compute the divisor
				Vector<Integer> newweights; //Contains the new weights
				Set<int> usedCones; //Contains the codim 1 cones with weight != 0
				Set<int> usedRays; //Contains the rays in used cones
				//Go through each facet and compute its weight. 
				for(int co = 0; co < codimOneCones.rows(); co++) {
					if(balancedFaces[co]) { //Only compute values at balanced codim-1-cones
						//dbgtrace << "Codim 1 face " << co << endl;
						Rational coweight(0); //Have to take rational since intermediate values may be rational
						Set<int> adjacentCones = coneIncidences.row(co);
						for(Entire<Set<int> >::iterator mc = entire(adjacentCones); !mc.at_end(); ++mc) {
							//dbgtrace << "Maximal cone " << *mc << endl;
							coweight = coweight + weights[*mc] * lnFunctionVector[std::make_pair(co,*mc)] * currentValues;
							//dbgtrace <<(lnFunctionVector[co])[*mc] * currentValues << endl; 
						}
						//Now substract the value of the lattice normal sum
						//dbgtrace << "Substracting sum" << endl;
						//dbgtrace << lsumFunctionVector.row(co) * currentValues << endl;
						coweight = coweight - lsumFunctionVector.row(co) * currentValues;
						if(coweight != 0) {
							//Invert weight sign for min people, the computation is rigged for max
							coweight *= (- Addition::orientation()); 
							newweights = newweights | Integer(coweight);	  
							usedCones += co;
							usedRays += codimOneCones.row(co);
						}
					}
				}//END iterate co-1-cones

				//dbgtrace << "Remaining " << usedCones.size() << " of " << codimOneCones.rows() << " codim one cones" << endl;
				//dbgtrace << "Removing " << sequence(0,codimOneCones.rows()) - usedCones << endl;

				//dbgtrace << "Computed codim one weights" << endl;
				//dbgtrace << "Weights are " << newweights << endl;

				//Compute the new-to-old maps used for recomputing the value vector in the next iteration
				if(r != values.rows()-1) {
					remainingFanRays = usedRays;
					newConesInOld = coneIncidences.minor(usedCones,All);	  
					result.give("SEPARATED_MAXIMAL_POLYTOPES") >> cmplx_oldcones; 
					result.give("SEPARATED_CONVERSION_VECTOR") >> old_conversion;
					newRaysToOldRays = Vector<int>();
					for(Entire<Set<int> >::iterator orays = entire(usedRays); !orays.at_end(); orays++) {
						newRaysToOldRays |= (*orays);
					}
				}
				//dbgtrace << "newConesInOld: " << newConesInOld << endl;
				//dbgtrace << "newRaysToOldRays:" << newRaysToOldRays << endl;

				//Now recompute the rays and maximal cones for re-initialization of the result
				rays = rays.minor(usedRays,All);
				weights = newweights;
				IncidenceMatrix<> newMaximal = codimOneCones.minor(usedCones,usedRays);
				//Recompute local restriction cones
				if(local_restriction.rows() > 0) {
					//We need to adapt rays indices and remove old maximal local cones
					// and codimension one cones that have weight 0
					//Also we remove all local cones that lose rays
					//dbgtrace << "Local restriction before: " << local_restriction << endl;
					IncidenceMatrix<> maxCones = result.give("MAXIMAL_CONES");
					Set<int> removableCones;
					Set<int> weightzerocones = sequence(0,codimOneCones.rows()) - usedCones;
					Set<int> codimToReplace; //Indices of used codim one cones that are local
					for(int lc = 0; lc < local_restriction.rows(); lc++) {
						//If the local cone loses any rays, remove it
						if((local_restriction.row(lc) * usedRays).size() < local_restriction.row(lc).size()) {
							removableCones += lc;
							continue;
						}
						bool found_cone = false;
						for(int mc = 0; mc < maxCones.rows(); mc++) {
							if((local_restriction.row(lc) * maxCones.row(mc)).size() == maxCones.row(mc).size()) {
								removableCones += lc;
								found_cone = true; break;
							}
						}
						for(Entire<Set<int> >::iterator cz = entire(weightzerocones); !cz.at_end() && !found_cone; cz++) {
							if((local_restriction.row(lc) * codimOneCones.row(*cz)).size() == codimOneCones.row(*cz).size()) {
								removableCones += lc;
								break;
							}
						}
					}

					//Remove cones
					local_restriction = local_restriction.minor(~removableCones, usedRays);
				
					//dbgtrace << "Adapted local cones: " << local_restriction << endl;
				}//END adapt local restriction	

				result = perl::Object(perl::ObjectType::construct<Addition>("Cycle"));
				result.take("PROJECTIVE_VERTICES") << rays;
				result.take("MAXIMAL_CONES") << newMaximal;
				result.take("WEIGHTS") << weights;
				result.take("LINEALITY_SPACE") << lineality_space;
				if(local_restriction.rows() > 0)
					result.take("LOCAL_RESTRICTION") << local_restriction;
				result.take("LATTICE_GENERATORS") << lattice_generators;
				lattice_bases = lattice_bases.minor(usedCones,All);
				result.take("LATTICE_BASES") << lattice_bases;//(lattice_bases.minor(usedCones,All));

			} //END iterate function rows

			//dbgtrace << "Done. Returning divisor" << endl;

			return result;
		}//END divisorByValueMatrix


	/**
	 *	@brief Computes the (k-fold) divisor of a RationalFunction on a given cycle
	 *	@param Cycle complex A tropical cycle
	 *	@param RationalFunction function A rational function, the cycle should be contained in
	 *	its domain (as a set, not as a polyhedral complex)
	 *	@param int k How often the function should be applied, 1 by default
	 *	@tparam Addition Min or Max.
	 *	@return Cycle The divisor.
	 */
	template <typename Addition>
		perl::Object divisor_with_refinement(perl::Object cycle, perl::Object function) {
			//Restrict the function to the cycle
			int power = function.give("POWER");
			perl::Object restricted_function = function.CallPolymakeMethod("restrict",cycle); 

			perl::Object domain = restricted_function.give("DOMAIN");
			//If the cycle had local restriction, we have to refine it as well
			if(cycle.exists("LOCAL_RESTRICTION")) {
				IncidenceMatrix<> ref_local = refined_local_cones(cycle, domain);
				domain = CallPolymakeFunction("local_restrict",domain, ref_local);
			}

			Vector<Rational> vertex_values = restricted_function.give("VERTEX_VALUES");
			Vector<Rational> lineality_values = restricted_function.give("LINEALITY_VALUES");
			Vector<Rational> full_values = vertex_values | lineality_values;

			Matrix<Rational> value_matrix(0, full_values.dim());
			for(int it = 1; it <= power; it++) {
				value_matrix /= full_values;
			}

			return divisorByValueMatrix<Addition>(domain,value_matrix);
		}//END divisor

	/**
	 * @brief Computes the divisor of a RationalFunction on a cycle which is supposed to be
	 * equal to the [[DOMAIN]] of the function (as a polyhedral complex!)
	 * (Note that [[DOMAIN]] needn't have weights, so we can't just take this.
	 */
	template <typename Addition>
		perl::Object divisor_no_refinement(perl::Object cycle, perl::Object function) {
			int power = function.give("POWER");
			Vector<Rational> vertex_values = function.give("VERTEX_VALUES");
			Vector<Rational> lineality_values = function.give("LINEALITY_VALUES");
			Vector<Rational> full_values = vertex_values | lineality_values;

			Matrix<Rational> value_matrix(0, full_values.dim());
			for(int it = 1; it <= power; it++) {
				value_matrix /= full_values;
			}

			return divisorByValueMatrix<Addition>(cycle,value_matrix);
		}

}}

#endif
