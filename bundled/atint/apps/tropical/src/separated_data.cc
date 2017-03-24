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

	The functions here deal with all the [[SEPARATED..]] properties	
	*/

#include "polymake/client.h"
#include "polymake/Set.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/Rational.h"
#include "polymake/Array.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/tropical/separated_data.h"


namespace polymake { namespace tropical {

	
	void computeSeparatedData(perl::Object X) {
		//Extract properties of X
		Matrix<Rational> rays = X.give("VERTICES");

		IncidenceMatrix<> codimOneCones = X.give("CODIMENSION_ONE_POLYTOPES");
		IncidenceMatrix<> maximalCones = X.give("MAXIMAL_POLYTOPES");
		IncidenceMatrix<> facet_incidences = X.give("MAXIMAL_AT_CODIM_ONE");
		facet_incidences = T(facet_incidences);

		//Result variables
		Matrix<Rational> cmplxrays(0,rays.cols());
		Vector<Set<int> > maxcones(maximalCones.rows());
		Vector<Set<int> > codimone(codimOneCones.rows());
		Vector<int> conversion;


		//Divide the set of rays into those with x0 != 0 and those with x0 = 0
		Set<int> affineRays;
		Set<int> directionalRays;
		Map<int,int> newAffineIndices; //This maps the old ray indices to the new ones in cmplxrays
		for(int r = 0; r < rays.rows(); r++) {
			if(rays.row(r)[0] == 0) {
				directionalRays = directionalRays + r;
			}
			else {
				affineRays = affineRays + r;
				cmplxrays = cmplxrays / rays.row(r);
				conversion |= r;
				newAffineIndices[r] = cmplxrays.rows()-1;
			}
		}
		
		//Insert the indices of the new affine rays for each cone
		for(int co = 0; co < codimOneCones.rows(); co++) {
			Set<int> corays = codimOneCones.row(co) * affineRays;
			codimone[co] = Set<int>();
			for(Entire<Set<int> >::iterator e = entire( corays); !e.at_end(); ++e) {
				codimone[co] = codimone[co] + newAffineIndices[*e];
			}
		}
		for(int mc = 0; mc < maximalCones.rows(); mc++) {
			Set<int> mcrays = maximalCones.row(mc) * affineRays;
			maxcones[mc] = Set<int>();
			for(Entire<Set<int> >::iterator e = entire( mcrays); !e.at_end(); ++e) {
				maxcones[mc] = maxcones[mc] + newAffineIndices[*e];
			}
		}

		//Now we go through the directional rays and compute the connected component for each one
		for(Entire<Set<int> >::iterator r = entire(directionalRays); !r.at_end(); ++r) {


			//List of connected components of this ray, each element is a component
			//containing the indices of the maximal cones
			Vector<Set<int> > connectedComponents;
			//The inverse of the component matrix, i.e. maps cone indices to row indices of connectedComponents
			Map<int,int> inverseMap;

			//Compute the set of maximal cones containing r
			Set<int> rcones;
			for(int mc = 0; mc < maximalCones.rows(); mc++) {
				if(maximalCones.row(mc).contains(*r)) {
					rcones = rcones + mc;
				}
			}


			//For each such maximal cone, compute its component (if it hasnt been computed yet).
			for(Entire<Set<int> >::iterator mc = entire(rcones); !mc.at_end(); ++mc) {
				if(!inverseMap.exists(*mc)) {
					//Create new component
					Set<int> newset; newset = newset + *mc;
					connectedComponents |= newset;
					inverseMap[*mc] = connectedComponents.dim()-1;

					//Do a breadth-first search for all other cones in the component
					std::list<int> queue;
					queue.push_back(*mc);
					//Semantics: Elements in that queue have been added but their neighbours might not
					while(queue.size() != 0) {
						int node = queue.front(); //Take the first element and find its neighbours
						queue.pop_front();
						for(Entire<Set<int> >::iterator othercone = entire(rcones); !othercone.at_end(); othercone++) {
							//We only want 'homeless' cones
							if(!inverseMap.exists(*othercone)) {
								//This checks whether both cones share a ray with x0=1
								if((maximalCones.row(node) * maximalCones.row(*othercone) * affineRays).size() > 0) {
									//Add this cone to the component
									connectedComponents[connectedComponents.dim()-1] += *othercone;
									inverseMap[*othercone] = connectedComponents.dim()-1;
									queue.push_back(*othercone);
								}
							}
						}
					}

				}
			} //END computation of connected components


			//Now add r once for each connected component to the appropriate cones
			for(int cc = 0; cc < connectedComponents.dim(); cc++) {
				cmplxrays = cmplxrays / rays.row(*r);
				conversion |= (*r);
				int rowindex = cmplxrays.rows()-1;
				Set<int> ccset = connectedComponents[cc];
				for(Entire<Set<int> >::iterator mc = entire(ccset); !mc.at_end(); ++mc) {
					maxcones[*mc] = maxcones[*mc] + rowindex;
					//For each facet of mc that contains r, add rowindex
					Set<int> fcset;
					//If there are maximal cones not intersecting x0 = 1, they have no facets
					//in facet_incidences, hence the following check
					if(*mc < facet_incidences.rows()) {
						fcset = facet_incidences.row(*mc);
					}
					for(Entire<Set<int> >::iterator fct = entire(fcset); !fct.at_end(); ++fct) {
						if(codimOneCones.row(*fct).contains(*r)) {
							codimone[*fct] = codimone[*fct] + rowindex;
						}
					}
				}
			}

		}//END iterate over all rays
	
		X.take("SEPARATED_VERTICES") << cmplxrays;
		X.take("SEPARATED_MAXIMAL_POLYTOPES") << maxcones;
		X.take("SEPARATED_CODIMENSION_ONE_POLYTOPES") << codimone;
		X.take("SEPARATED_CONVERSION_VECTOR") << conversion;

	}//END computeSeparatedData

	Function4perl(&computeSeparatedData,"computeSeparatedData(Cycle)");

}}
