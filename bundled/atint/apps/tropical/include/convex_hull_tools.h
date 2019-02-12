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

	This defines wrapper / helper functions for using the polymake-convex-hull-interface
	*/

#ifndef POLYMAKE_ATINT_CONVEX_HULL_TOOLS_H
#define POLYMAKE_ATINT_CONVEX_HULL_TOOLS_H

#include "polymake/Matrix.h"
#include "polymake/Rational.h"
#include "polymake/Vector.h"
#include "polymake/IncidenceMatrix.h"

namespace polymake { namespace tropical {

struct fan_intersection_result {
  Matrix<Rational> rays;
  Matrix<Rational> lineality_space;
  IncidenceMatrix<> cones;
  IncidenceMatrix<> xcontainers;
  IncidenceMatrix<> ycontainers;
};


/**
   @brief Inserts a list of new rays into a matrix of existing rays, taking care of doubles. This method DOES change the matrix of existing rays.
   @param Matrix<Rational> rays A reference to a matrix of (normalized) rays or vertices. This matrix will potentially be changed
   @param Matrix<Rational> nrays A list of new rays or vertices (not necessarily normalized), that will be added
   @param bool is_normalized Whether the new rays are also already normalized
   @return Vector<int> At position i contains the row index of the new ray nrays[i] in the modified matrix rays
*/
Vector<int> insert_rays(Matrix<Rational> &rays, Matrix<Rational> nrays, bool is_normalized );

/**
   @brief Normalizes a ray matrix: Vertices begin with a 1 and the first non-zero coordinate of a ray is +-1
   @param Matrix The row vectors to be normalized. This method modifies this matrix!
*/
template <typename MType>
void normalize_rays(GenericMatrix<MType>& rays);

/**
   @brief Computes the intersection of two rational polyhedra, x and y, given in terms of rays and lineality space
   @param Matrix<Rational> xrays The rays of x, non-tropically homogeneous.
   @param Matrix<Rational> xlin The lineality space of x, ditto.
   @param Matrix<Rational> yrays The rays of y, ditto.
   @param Matrix<Rational> ylin The lineality space of y, ditto
   @return An std::pair of Matrix<Rational>, containing first rays, then lineality space of the intersection. The rays are normalized
*/
std::pair<Matrix<Rational>, Matrix<Rational>> cone_intersection(
   const Matrix<Rational>& xrays, const Matrix<Rational>& xlin,
   const Matrix<Rational>& yrays, const Matrix<Rational>& ylin);

/**
   @brief Computes the set-theoretic intersection of two polyhedral complexes, x and y. All coordinates should
   be given non-tropically-homogeneously.
   @param Matrix<Rational> xrays The rays of the complex x
   @param Matrix<Rational> xlin The lineality space of the complex x
   @param IncidenceMatrix<> xcones The cones of the complex x, in terms of xrays
   @param Matrix<Rational> yrays The rays of the complex y
   @param Matrix<Rational> ylin The lineality space of the complex y
   @param IncidenceMatrix<> ycones The cones of the complex y, in terms of yrays
   @return The set-theoretic intersection of x and y in a struct containing:
   1) Matrix<Rational> rays The rays of the intersection complex.
   2) Matrix<Rational> lineality The lineality space of the complex
   3) IncidenceMatrix<> cones The maximal cones of the complex. More precisely: All pairwise intersections s \cap t of cones s in x, t in y. In particular, cones in this matrix might be contained in one another (but never equal). This list will not contain the empty cone
   4) IncidenceMatrix<> xcontainers For the i-th cone of the intersection, the i-th row gives the indices of all maximal cones in x containing it
   5) IncidenceMatrix<> ycontainers For the i-th cone of the intersection, the i-th row gives the indices of all maximal cones in y containing it
*/
fan_intersection_result fan_intersection(
   const Matrix<Rational>& xrays, const Matrix<Rational>& xlin, const IncidenceMatrix<>& xcones,
   const Matrix<Rational>& yrays, const Matrix<Rational>& ylin, const IncidenceMatrix<>& ycones);

} }

#endif
