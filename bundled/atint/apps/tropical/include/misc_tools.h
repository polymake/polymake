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
	Copyright (c) 2016-2021
	Ewgenij Gawrilow, Michael Joswig, and the polymake team
	Technische Universität Berlin, Germany
	https://polymake.org

	Contains some miscellaneous tools.

*/


#pragma once

#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/Set.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Array.h"
#include "polymake/RandomGenerators.h"
#include "polymake/linalg.h"
#include "polymake/tropical/homogeneous_convex_hull.h"

namespace polymake { namespace tropical {

/**
   @brief Takes a matrix and returns the row indices where the first coordinate is nonzero and where the first coordinate is zero in  two different sets
   @param Matrix<Rational> m The matrix whose rows we consider
   @return std::pair<Set<Int>, Set<Int>> The first set contains the row indices of rows that start with a zero entry, the second set is the complement
*/
template <typename MType>
std::pair<Set<Int>, Set<Int>> far_and_nonfar_vertices(const GenericMatrix<MType>& m)
{
  const auto& first_col_supp = support(m.col(0));
  return std::pair<Set<Int>, Set<Int>>( sequence(0, m.rows()) - first_col_supp, first_col_supp);
}

/**
 * @brief Takes a polyhedral complex and returns [[CONES]] summarized into one single incidence matrix.
 * @param PolyhedralComplex
 * @return IncidenceMatrix<>
 */
IncidenceMatrix<> all_cones_as_incidence(BigObject complex);

/**
 * @brief Converts an incidence matrix to a Vector<Set<Int>>
 */
template <typename MType>
Vector<Set<Int>> incMatrixToVector(const GenericIncidenceMatrix<MType>& i)
{
  return Vector<Set<Int>>(i.rows(), entire(rows(i)));
}

inline
Vector<Integer> randomInteger(const Int max_arg, const Int n)
{
  static UniformlyRandomRanged<Integer> rg(max_arg);
  return Vector<Integer>(n, rg.begin());
}

/**
   @brief Computes all vectors of dimension n with entries +1 and -1.
   They are sorted such that each vector v has the row index determined by the sum:
   sum_{i: v_i = 1} 2^i (where i runs from 0 to n-1)
   @param Int n The column dimension of the matrix
   @return Matrix<Rational> A 2^n by n matrix containing all +-1-vectors of dimension n
*/
Matrix<Rational> binaryMatrix(Int n);

/**
   @brief Assumes v is a vector with entries +1 and -1 only.
   Returns sum_{i: v_i = 1} 2^i (where i runs from 0 to n-1
*/
template <typename VType>
Int binaryIndex(const GenericVector<VType>& v)
{
  Int result = 0;
  for (const Int i : indices(attach_selector(v.top(), operations::positive())))
    result += pow(2,i);
  return result;
}

/**
   @brief Helper function for the refinement function.
   Given a polyhedral cell in terms of rays and lineality space, it computes, whether a given ray
   is contained in this cell (possibly modulo (1,..,1)).
   @param Matrix<Rational> rays The rays of the cell
   @param Matrix<Rational> lineality The lineality space of the cell
   @param Vector<Rational> ray The ray to be tested
   @param bool is_projective Whether coordinates are given as tropical projective coordinates.
          (False means they're affine).
   @param solver A convex hull solver
   @returns true, if and only if ray lies in the cone
*/
inline
bool is_ray_in_cone(const Matrix<Rational>& rays, const Matrix<Rational>& lineality,
                    const Vector<Rational>& ray, const bool is_projective)
{
  const auto facets = is_projective ? enumerate_homogeneous_facets(rays, lineality)
                                    : polytope::enumerate_facets(rays, lineality, false);
  // Check equations
  for (auto l = entire(rows(facets.second)); !l.at_end(); ++l) {
    if (*l * ray != 0) return false;
  }
  // Check facets
  for (auto f = entire(rows(facets.first)); !f.at_end(); ++f) {
    if (*f * ray < 0) return false;
  }
  return true;
}

} }

