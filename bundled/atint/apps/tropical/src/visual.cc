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

	Computes the data for BB_VISUAL
	*/


#include "polymake/client.h"
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/linalg.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/tropical/LoggingPrinter.h"
#include "polymake/tropical/solver_def.h"

namespace polymake { namespace tropical {


	using namespace atintlog::donotlog;
	//   using namespace atintlog::dolog;
	//using namespace atintlog::dotrace;
	
	/**
	  @brief This takes a list of vertices (as row vectors, without leading ones) 
	  and computes the corresponding bounding box, 
	  i.e. it computes the minima /maxima over all coordinates, subtracts/adds a given distance 
	  and returns the resulting 2x(no of coordinates)-matrix. 
	  The first row contains the min-coords, the second the max-coords
	  @param Matrix<Rational> rays The vertices
	  @param Rational distance The minimal distance the box should have from all vertices
	  @param Bool make_it_a_cube If this is true (it is false by default), the bounding box is made
	  a cube, by setting all coordinates to be the respective maximum / minimum
	  */
	Matrix<Rational> boundingBox(Matrix<Rational> rays, Rational distance, bool make_it_a_cube = 0) {
		if(rays.rows() == 0) return Matrix<Rational>(2,rays.cols());
		Vector<Rational> min_values = rays.row(0);
		Vector<Rational> max_values = rays.row(0);
		for(int c = 0; c < rays.cols(); c++) {
			min_values[c] = accumulate( rays.col(c), operations::min());
			max_values[c] = accumulate( rays.col(c), operations::max());
		}
		if(make_it_a_cube) {
			Rational total_max = accumulate( max_values, operations::max());
			Rational total_min = accumulate( min_values, operations::min());
			max_values = ones_vector<Rational>(rays.cols()) * total_max;
			min_values = ones_vector<Rational>(rays.cols()) * total_min;
		}
		//Add distance
		min_values -= distance* ones_vector<Rational>(rays.cols());
		max_values += distance* ones_vector<Rational>(rays.cols());
		return min_values / max_values;
	}//END boundingBox

	/**
	  @brief Computes the polyhedral data necessary for visualization with a bounding box
	  @param fan::PolyhedralComplex fan The polyhedral complex to be visualized, in 
	  non-tropically-homogeneous coordinates
	  @param Vector<Integer> weights The weights of the cycle. If not empty, 
	  the barycenters of the polytopes are computed for weight labelling
	  @param Matrix<Rational> bBox The bounding box. 
	  Given by two row vectors indicating the extreme points of the box
	  @param Array<String> clabels If showWeights is false and this array has positive length, 
	  these strings will be used to label the maximal cones (missing labels are replaced by the emtpy string)
	  @return A perl::ListReturn containing 
	  1) the list of polytopes to be rendered
	  2) A polytope::PointConfiguration that will contain the center of each cell as vertex, 
	  labelled with the corresponding weight. 
	  This is only computed if showWeights is true, but is contained in the ListReturn in any case.
	  */
	perl::ListReturn computeBoundedVisual(perl::Object fan, Vector<Integer> weights, 
			Matrix<Rational> bBox, Array<std::string> clabels) {
		//Extract values
		int ambient_dim = fan.CallPolymakeMethod("AMBIENT_DIM");
		Matrix<Rational> rays = fan.give("VERTICES");
		Set<int> far_rays = fan.give("FAR_VERTICES");
		Set<int> vertices = sequence(0,rays.rows()) - far_rays;
		IncidenceMatrix<> maximalCones = fan.give("MAXIMAL_POLYTOPES");
		Matrix<Rational> facetNormals = fan.give("FACET_NORMALS");
		Matrix<Rational> facetNormalsInCones = fan.give("MAXIMAL_POLYTOPES_FACETS");
		Matrix<Rational> linearSpan = fan.give("LINEAR_SPAN_NORMALS");
		IncidenceMatrix<> linearSpanInCones = fan.give("MAXIMAL_POLYTOPES_AFFINE_HULL_NORMALS");
		int fan_dim = fan.CallPolymakeMethod("DIM");
		bool showWeights = weights.dim() != 0;

		bool use_labels = !showWeights && clabels.size() > 0;

		//First separate affine and directional rays
		Set<int> affineRays;
		Set<int> directionalRays;
		for(int r = 0; r < rays.rows(); r++) {
			if(rays.row(r)[0] == 0) {
				directionalRays = directionalRays + r;
			}
			else {
				affineRays = affineRays + r;
			}
		}  

		//dbgtrace << "Computing bounding box..." << endl;

		//Compute facets of the bounding box
		Matrix<Rational> bbFacets(0,ambient_dim+1);

		//For each coordinate, contain minimum and maximum
		Vector<Rational> minCoord(ambient_dim);
		Vector<Rational> maxCoord(ambient_dim);
		for(int i = 0; i < ambient_dim; i++) {
			maxCoord[i] = bBox(0,i) > bBox(1,i)? bBox(0,i) : bBox(1,i);
			minCoord[i] = bBox(0,i) < bBox(1,i)? bBox(0,i) : bBox(1,i);
		}
		//Now make these coordinates into facets
		for(int i = 0; i < ambient_dim; i++) {
			Vector<Rational> facetVector = unit_vector<Rational>(ambient_dim,i);
			bbFacets /= (maxCoord[i] | -facetVector);
			bbFacets /= (-minCoord[i] | facetVector);
		}

		//dbgtrace << "Done." << endl;

		perl::ListReturn result;

		//This will contain the cell centers with the weight labels
		perl::Object weightCenters("polytope::PointConfiguration");
		Matrix<Rational> centermatrix(0,ambient_dim);
		Vector<std::string> centerlabels;

		solver<Rational> sv;
		//Now compute all polyhedra to be rendered
		for(int mc = 0; mc < maximalCones.rows(); mc++) {
			//dbgtrace << "Computing polytope of cone " << mc << endl;
			//Compute the facets ans equalities of the current cone and add the bbox facets
			Matrix<Rational> facets(0,ambient_dim+1);
			Matrix<Rational> linspan = linearSpan.minor(linearSpanInCones.row(mc),All);
			linspan = linspan;
			for(int fn = 0; fn < facetNormalsInCones.cols(); fn++) {
				if(facetNormalsInCones(mc,fn) == 1) {
					facets /= facetNormals.row(fn);
				}
				if(facetNormalsInCones(mc,fn) == -1) {
					facets /= (-facetNormals.row(fn));
				}
			}
			facets /= bbFacets;
			//facets = facets;

			//dbgtrace << "Facets are " << facets << "Equalities are " << linspan << endl;

			//Compute the polytope vertices from that
			Matrix<Rational> polyRays;
			try {
				polyRays = solver<Rational>().enumerate_vertices(facets, linspan, false,true).first;
			}
			catch(...) {
				polyRays = Matrix<Rational>(0,ambient_dim);
			}
			//Normalize
			for(int r = 0; r < polyRays.rows(); r++) {
				if(polyRays(r,0) != 0) polyRays.row(r) /= polyRays(r,0);
			}
			//We have to make sure that the polytope has
			//at least dim +1 vertices after cutting, otherwise its a point set or graph to the
			//visualization and all the Facet options don't work
			if(polyRays.rows() >= fan_dim+1) {
				perl::Object polytope("polytope::Polytope<Rational>");
				polytope.take("VERTICES") << polyRays; //The polytope shouldn't have a lineality space
				result << polytope;

				//If weight labels should be displayed, compute the vertex barycenter of the polytope and
				// label it
				if(showWeights || use_labels) {
					Vector<Rational> barycenter = average(rows(polyRays));
					//barycenter /= barycenter[0];
					centermatrix = centermatrix / barycenter;
					std::ostringstream wlabel;
					wlabel << "# " << mc << ": ";
					if(showWeights) wlabel << weights[mc];
					else {
						if(mc < clabels.size()) wlabel << clabels[mc];
					}
					centerlabels |= wlabel.str();
				}
			}

		}//END iterate rendered polyhedra

		if(showWeights || use_labels) {
			weightCenters.take("POINTS") << centermatrix;
			weightCenters.take("LABELS") << centerlabels;
		}
		result << weightCenters;

		return result;
	}
	// PERL WRAPPER /////////////////////////////////////////////////////

	Function4perl(&boundingBox,"boundingBox(Matrix;$=1,$=0)");
	Function4perl(&computeBoundedVisual,"computeBoundedVisual(fan::PolyhedralComplex, Vector<Integer>, Matrix<Rational>, Array<String>)");
}}
