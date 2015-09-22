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

	Implements codim_one_with_locality.h
	*/

#include "polymake/tropical/codim_one_with_locality.h"
#include "polymake/tropical/solver_def.h"
#include "polymake/tropical/separated_data.h"
#include "polymake/tropical/LoggingPrinter.h"

namespace polymake { namespace tropical {

	
	using namespace atintlog::donotlog;
	// using namespace atintlog::dolog;
	// using namespace atintlog::dotrace;
	
		
	CodimensionOneResult calculateCodimOneData(const Matrix<Rational> &rays, const IncidenceMatrix<> &maximalCones, const Matrix<Rational> &linspace, const IncidenceMatrix<>  &local_restriction) {

		//dbgtrace << "Computing all facets..." << endl;

		//First we construct the set of all facets 
		//Array<IncidenceMatrix<> > maximal_cone_incidence = fan.give("MAXIMAL_CONES_INCIDENCES");
		//Compute the rays-in-facets for each cone directly
		Vector<IncidenceMatrix<> > maximal_cone_incidence;
		for(int mc = 0; mc < maximalCones.rows(); mc++) {
			Set<int> mset = maximalCones.row(mc);
			//Extract inequalities
			//dbgtrace << "Computing facets for cone set " << mset << endl;
			Matrix<Rational> facets = solver<Rational>().enumerate_facets(
					rays.minor(mset,All),
					linspace,false,false).first;
			//dbgtrace << "Done. Checking rays..." << endl;
			//For each inequality, check which rays lie in it
			Vector<Set<int> > facetIncidences;
			for(int row = 0; row < facets.rows(); row++) {
				Set<int> facetRays;
				for(Entire<Set<int> >::iterator m = entire(mset); !m.at_end(); m++) {
					if(facets.row(row) * rays.row(*m) == 0) {
						facetRays += *m;
					}
				}
				facetIncidences |= facetRays;
			}
			//dbgtrace << "Done." << endl;
			maximal_cone_incidence |= IncidenceMatrix<>(facetIncidences);
		}

		//dbgtrace << "Check for doubles and useless facets..." << endl;

		//This will contain the set of indices defining the codim one faces
		Vector<Set<int> > facetArray;

		//This will define the codim-1-maximal-cone incidence matrix
		Vector<Set<int> > fIncones;

		for(int maxcone = 0; maxcone < maximal_cone_incidence.size(); maxcone++) {
			//This is the incidence matrix for the maximal cone indexed by maxcone
			IncidenceMatrix<> fcts = maximal_cone_incidence[maxcone];
			for(int facet = 0; facet < fcts.rows(); facet++) {
				Set<int> facetToCheck = fcts.row(facet);
				//If there is a local restriction, check if the facet is compatible
				if(local_restriction.rows() > 0) {
					if(!is_coneset_compatible(facetToCheck, local_restriction)) continue;
				}
				//Check if this facet intersects x0 = 1, otherwise go to the next one 
				//More precisely: Check if at least one of its rays has x0-coord != 0
				Vector<Rational> firstColumn = rays.minor(facetToCheck,All).col(0);
				if(firstColumn == zero_vector<Rational>(firstColumn.dim())) {
					continue;
				}
				//Otherwise check if we already have that facet and remember its index
				int fcIndex = -1;
				for(int existing = 0; existing < facetArray.dim(); existing++) {
					if(facetArray[existing] == facetToCheck) {
						fcIndex = existing;
						break;
					}
				}
				//Add the facet if necessary and add its maximal-cone indices
				if(fcIndex == -1) {
					facetArray |= facetToCheck;
					Set<int> singlecone;
					singlecone = singlecone + maxcone;
					fIncones |= singlecone;
				}
				else {
					fIncones[fcIndex] = fIncones[fcIndex] + maxcone;
				}
			}
		}

		CodimensionOneResult r;
		r.codimOneCones = IncidenceMatrix<>(facetArray);
		r.codimOneInMaximal = IncidenceMatrix<>(fIncones);
		return r;    
	}

	/*
	 * @brief Computes [[CODIMENSION_ONE_POLYTOPES]], [[MAXIMAL_AT_CODIM_ONE]] 
	 * and [[FACET_NORMALS_BY_PAIRS]] for cycles with [[LOCAL_RESTRICTION]]
	 */
	template <typename Addition>
	void codim_one_with_locality(perl::Object cycle) {
		Matrix<Rational> rays = cycle.give("VERTICES");
		IncidenceMatrix<> cones = cycle.give("MAXIMAL_POLYTOPES");
		IncidenceMatrix<> local_restriction = cycle.give("LOCAL_RESTRICTION");
		Matrix<Rational> lineality = cycle.give("LINEALITY_SPACE");

		//Create a proxy object without the local restriction
		perl::Object proxy(perl::ObjectType::construct<Addition>("Cycle"));
			proxy.take("PROJECTIVE_VERTICES") << rays;
			proxy.take("MAXIMAL_POLYTOPES") << cones;
			proxy.take("LINEALITY_SPACE") << lineality;

		//Extract non-local data
		IncidenceMatrix<> codim_one = proxy.give("CODIMENSION_ONE_POLYTOPES");
		IncidenceMatrix<> maximal_at_codim = proxy.give("MAXIMAL_AT_CODIM_ONE");
		Map<std::pair<int,int>, int> facets_by_pairs = proxy.give("FACET_NORMALS_BY_PAIRS");

		//Find non-local codim one cones
		Set<int> nonlocal;
		Map<int,int> new_codim_index; //Maps old codim indices to new ones
		int next_index = 0;
		for(int cc = 0; cc < codim_one.rows(); cc++) {
			if(!is_coneset_compatible(codim_one.row(cc),local_restriction)) {
				nonlocal += cc;
			}
			else {
				new_codim_index[cc] = next_index;
				next_index++;
			}
		}

		//Clean map
		Map<std::pair<int,int>, int> local_map;
		for(Entire<Map<std::pair<int,int>,int > >::iterator fct = entire(facets_by_pairs); !fct.at_end(); fct++) {
			if(!nonlocal.contains( (*fct).first.first))
					local_map[std::make_pair(new_codim_index[(*fct).first.first],(*fct).first.second )] = (*fct).second;
		}

		cycle.take("CODIMENSION_ONE_POLYTOPES") << codim_one.minor(~nonlocal,All);
		cycle.take("MAXIMAL_AT_CODIM_ONE") << maximal_at_codim.minor(~nonlocal,All);
		cycle.take("FACET_NORMALS_BY_PAIRS") << local_map;
	
	}

	

	FunctionTemplate4perl("codim_one_with_locality<Addition>(Cycle<Addition>) : void");

}}

