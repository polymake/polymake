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

	Defines various data types for the lines_in_cubic routine.
	*/

#ifndef POLYMAKE_ATINT_LINES_IN_CUBIC_DATA_H
#define POLYMAKE_ATINT_LINES_IN_CUBIC_DATA_H

#include "polymake/client.h"
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/IncidenceMatrix.h"

namespace polymake { namespace tropical {

	/**
	 * This contains the intersection of two ReachableResults:
	 * - The rays of the complex
	 * - The maximal two-dimensional cells
	 * - The maximal one-dimensional cells
	 * - The maximal zero-dimensional cells
	 */
	struct DirectionIntersection {
		Matrix<Rational> rays;
		IncidenceMatrix<> cells;
		IncidenceMatrix<> edges;
		IncidenceMatrix<> points;    
	};

	/**
	 * This contains the facet data of a two-dimensional cone in R^3:
	 * - The codimension one faces in terms of ray indices (in a ray matrix that was given when creating this object)
	 * - In the same order as the faces, a list of the corresponding inequalities in std polymake format, given as a matrix
	 * - The single equation of the cone, given as a vector
	 */
	struct FacetData {
		IncidenceMatrix<> facets;
		Matrix<Rational> ineqs;
		Vector<Rational> eq;
	};

	/**
	 * Describes a line with a single vertex or a family starting in such a line:
	 * - The vertex of the line
	 * - A list of indices in 0,..,5 indicating which rays span a 2-dimensional cell in the family:
	 * 	0 = 0,1
	 * 	1 = 0,2
	 * 	2 = 0,3
	 * 	3 = 1,2
	 * 	4 = 1,3
	 * 	5 = 2,3
	 */
	struct VertexLine {
		Vector<Rational> vertex;
		Set<int> cells;
	};

	/**
	 * Describes a one-dimensional family of a line with a single vertex:
	 * - A matrix (with 2 rows) describing the edge of the family 
	 */
	struct VertexFamily {
		Matrix<Rational> edge;
	};

	/**
	 * Describes a single line with a bounded edge or a family starting at such a line:
	 * - The vertex at 0
	 * - The vertex away from 0
	 * - The vertex furthest away from 0-vertex in direction of the edge and still in X
	 * - The vertex furthest away from not-0-vertex in direction of the edge and still in X
	 * - The index of the other leaf at 0
	 * - Whether the leafs at 0 span a cell
	 * - Whether the leafs away from 0 span a cell
	 */
	struct EdgeLine {
		Vector<Rational> vertexAtZero;
		Vector<Rational> vertexAwayZero;
		Vector<Rational> maxDistAtZero;
		Vector<Rational> maxDistAwayZero;
		int leafAtZero;
		bool spanAtZero;
		bool spanAwayZero;
	};

	/**
	 * Describes a family of lines with bounded edge:
	 * - A list of matrices (with 2 rows) describing the edges at 0
	 * - A list of matrices (with 2 rows) describing the edges away from 0 (should have 
	 *  the same number of elements as the first list)
	 * - The border vertices (or rays) at 0
	 * - The border vertices (or rays) away zero
	 * - The index of the other leaf at 0
	 */
	struct EdgeFamily {
		Vector< Matrix<Rational> > edgesAtZero;
		Vector< Matrix<Rational> > edgesAwayZero;
		Matrix<Rational> borderAtZero;
		Matrix<Rational> borderAwayZero;
		Matrix<Rational> center;
		int leafAtZero;
	};

}}

#endif

