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
	Copyright (c) 2016-2020
	Ewgenij Gawrilow, Michael Joswig, and the polymake team
	Technische Universität Berlin, Germany
	https://polymake.org

	Converts matrices and translates of Morphism objects from projective to affine
	coordinates and back
	*/

#ifndef POLYMAKE_ATINT_MORPHISM_THOMOG_H
#define POLYMAKE_ATINT_MORPHISM_THOMOG_H

#include "polymake/client.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/Rational.h"

namespace polymake { namespace tropical {

/*
 * @brief Takes a matrix and a translate defining an affine linear map on given charts
 * and converts it into a homogeneous representation.
 * @return Pair<Matrix,Translate> 
 */
std::pair<Matrix<Rational>, Vector<Rational>>
thomog_morphism(const Matrix<Rational>& matrix, const Vector<Rational>& translate, Int domain_chart = 0,
                Int target_chart = 0); 

/*
 * @brief Takes a homogeneous representation of a Morphism and converts it into a non-homogenous
 * one on given charts.
 */
std::pair<Matrix<Rational>, Vector<Rational>>
tdehomog_morphism(const Matrix<Rational>& matrix, const Vector<Rational>& translate, Int domain_chart = 0,
                  Int target_chart = 0);

/*
 * @brief Test if the columns of a matrix add up to a multiple of the all-ones-vector.
 */
bool is_homogeneous_matrix(const Matrix<Rational>& m);

} }

#endif
