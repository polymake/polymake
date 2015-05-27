/* Copyright (c) 1997-2015
   Ewgenij Gawrilow, Michael Joswig (Technische Universitaet Berlin, Germany)
   http://www.polymake.org

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version: http://www.gnu.org/licenses/gpl.txt.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
--------------------------------------------------------------------------------
*/

#include "polymake/client.h"
#include "polymake/Rational.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/Set.h"

namespace polymake { namespace tropical {
	
	/*
	 * @brief Computes the set of indices such that the corresponding entries
	 * in a given vector are either all zero or nonzero.
	 * @param Vector v The vector
	 * @param bool search_zeros: If true, find all zero entries, if false all non-zero
	 * entries
	 * @return Set<int>
	 */
	template <typename T>
	Set<int> binaryFinder(const GenericVector<T> &v, bool search_zeros) {
		if(search_zeros) return indices( attach_selector(v.top(), operations::is_zero()));
		else return indices( attach_selector(v.top(), operations::non_zero())); 
	}

	/*
	 * @brief Computes the properties [[CODIMENSION_ONE_POLYTOPES]],
	 * [[MAXIMAL_AT_CODIM_ONE]] and [[FACET_NORMALS_BY_PAIRS]] of a Cycle.
	 * FIXME: When fan's hasse_diagram has been specialized to pure fans, 
	 * we should use that!
	 */
	void compute_codimension_one_polytopes(perl::Object X) {
		//Extract properties
		IncidenceMatrix<> maximalCones = X.give("MAXIMAL_POLYTOPES");
		Matrix<Rational> vertices = X.give("VERTICES");
		Matrix<int> maximalFacets = X.give("MAXIMAL_POLYTOPES_FACETS");
		Matrix<Rational> facets = X.give("FACET_NORMALS");
		Set<int> far_vertices = X.give("FAR_VERTICES");
		
		//For each facet, compute the vertices in it
		Vector<Set<int> > vertices_in_facets(facets.rows());
		for(int f = 0; f  < facets.rows(); f++) {
			Vector<Rational> vertTimesFacet = vertices * facets.row(f);
			vertices_in_facets[f] = binaryFinder(vertTimesFacet, true);
		}

		//For each maximal cone, find its facets and intersect the vertex sets		
		Vector<Set<int> > codim_one_cones;
		Vector<Set<int> > maximal_at_codim;
		Map<std::pair<int,int>,int> pair_mapper;

		//If MAXIMAL_POLYTOPES only contains the empty cone, it is a 1x0 matrix
		//MAXIMAL_POLYTOPES_FACETS will then also be a 1x0 matrix, but c++-side will 
		//see it as a 0x0 matrix

		for(int m = 0; m < std::min(maximalCones.rows(),maximalFacets.rows()); m++) {
			Set<int> facetsOfM = binaryFinder(maximalFacets.row(m),false);
			for(Entire<Set<int> >::const_iterator mfacet = entire(facetsOfM); !mfacet.at_end(); mfacet++) {
				Set<int> newFacet = maximalCones.row(m) * vertices_in_facets[*mfacet];
				//Make sure the facet has at least one non-far vertex
				if( (newFacet - far_vertices).size() == 0) continue;
				//Check if this codim one one already exists
				int index = -1;
				for(int ex = 0; ex < codim_one_cones.dim(); ex++) {
					if(codim_one_cones[ex] == newFacet) {
						index = ex; break;
					}
				}
				if(index >= 0) {
					maximal_at_codim[index] += scalar2set(m);
					pair_mapper[std::pair<int,int>(index,m)] = *mfacet;
				}
				else {
					codim_one_cones |= newFacet;
					maximal_at_codim |= scalar2set(m);
					pair_mapper[std::pair<int,int>(codim_one_cones.dim()-1,m)] = *mfacet;
				}

			}
		}

		X.take("CODIMENSION_ONE_POLYTOPES") << codim_one_cones;
		X.take("MAXIMAL_AT_CODIM_ONE") << maximal_at_codim;
		X.take("FACET_NORMALS_BY_PAIRS") << pair_mapper;
	}

	Function4perl(&compute_codimension_one_polytopes,"compute_codimension_one_polytopes(Cycle)");

}}
