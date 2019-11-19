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

	---
	Copyright (c) 2016-2019
	Ewgenij Gawrilow, Michael Joswig, and the polymake team
	Technische Universität Berlin, Germany
	https://polymake.org

	Contains several small helper functions for the lines_in_cubic routine.
	*/

#ifndef POLYMAKE_ATINT_LINES_IN_CUBIC_HELPER_H
#define POLYMAKE_ATINT_LINES_IN_CUBIC_HELPER_H

#include "polymake/client.h"
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/linalg.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/tropical/divisor.h"
#include "polymake/tropical/convex_hull_tools.h"
#include "polymake/tropical/lines_in_cubic_data.h"

namespace polymake { namespace tropical {

/**
 * Returns the result of computeEdgeFamilies:
 * - A vector of EdgeFamily objects
 * - A vector of EdgeLine objects
 * - A vector of VertexLine objects
 */
struct LinesInCellResult {
  Vector<EdgeFamily> edge_families;
  Vector<EdgeLine> edge_lines;
  Vector<VertexLine> vertex_lines;
};

/**
   @brief Takes a fan_intersection_result and cleans up the result so that no cone is contained in another and that cones are sorted according to their dimension. 
   @param fan_intersection_result fir
   @return DirectionIntersection
*/
DirectionIntersection cleanUpIntersection(const fan_intersection_result& fir);

/**
   @brief This takes a (two-dimensional) cone in R^3 in terms of a subset of rays and computes all codimension one faces. 
   @return A FacetData object (see above)
*/
FacetData computeFacets(const Matrix<Rational>& rays, const Set<int>& cone);

/**
   @brief This takes a vertex in the cubic (whose function's domain is describes by frays and fcones) and a direction and computes the vertex w farthest away from vertex in this direction, such that the convex hull of vertex and w still lies in X. It returns the empty vertex, if the complete half-line lies in X.
*/
Vector<Rational> maximalDistanceVector(const Vector<Rational>& vertex, const Vector<Rational>& direction, 
                                       const Matrix<Rational>& frays, const IncidenceMatrix<>& fcones,
                                       const Matrix<Rational>& funmat);

/**
   @brief Takes two distinct(!) vectors v1 and v2 and a standard direction e_i + e_j for i,j in {0,..,3} and computes the rational number r, such that v1 + r*direction = v2. Returns null, if no such number exists
*/
Rational vertexDistance(const Vector<Rational>& v1, const Vector<Rational>& v2, const Vector<Rational>& direction);

/**
   @brief Takes a vertex family and computes the index of the standard direction in 0,..,3 corresponding to its edge
*/
int vertexFamilyDirection(const VertexFamily& f);

/**
   @brief Computes all edge families lying in a 2-dimensional cone for a given direction
   @param DirectionIntersection cone A cone, refined along f
   @param Matrix<Rational> z_border The intersection of the cone with a cone in the 0-i-rechable locus
   @param Matrix<Rational> c_border The intersection of the cone with a cone in the j-k-reachable locus
   @param int leafAtZero The index of the leaf together with 0
   @param Matrix<Rational> funmat The function matrix of f, made compatible for vector multiplication
   @return LinesInCellResult A list of all (families of) lines lying in the cone	    
*/
LinesInCellResult computeEdgeFamilies(const DirectionIntersection& cone, 
                                      const Matrix<Rational>& z_border,
                                      const Matrix<Rational>& c_border,
                                      int leafAtZero,
                                      const Matrix<Rational>& funmat);
} }

#endif

