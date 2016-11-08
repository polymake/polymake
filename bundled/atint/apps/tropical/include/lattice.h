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

	Basic functionality concerning lattices.
	*/


#ifndef POLYMAKE_ATINT_LATTICE_H
#define POLYMAKE_ATINT_LATTICE_H

#include "polymake/Rational.h"
#include "polymake/Matrix.h"

namespace polymake { namespace tropical {

	/*
	 * @brief Computes the lattice basis of a cone (given in not-tropically-homogeneous coordinates 
	 * without leading coordinate) and whose dimension is known.
	 * @param Matrix<Rational> A list of rays
	 * @param Matrix<Rational> generators for the lineality space
	 * @param Int the dimension of the cone 
	 * @param Bool whether the cone has a leading coordinate (and is, in fact, a polyhedron).
	 * @return A lattice basis, given as row vectors of a matrix and without leading coordinate.
	 */
	Matrix<Integer> lattice_basis_of_cone(const Matrix<Rational> &rays, const Matrix<Rational> &lineality, int dim, bool has_leading_coordinate);

}}

#endif
