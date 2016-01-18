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

	Deals with computing values of morphisms from matrix representations and vice versa.
	*/

#ifndef POLYMAKE_ATINT_MORPHISM_VALUES_H 
#define POLYMAKE_ATINT_MORPHISM_VALUES_H

#include "polymake/Matrix.h"
#include "polymake/Rational.h"
#include "polymake/Vector.h"

namespace polymake { namespace tropical {

	/**
	  @brief Takes an affine linear function defined on a polyhedron given by values on the rays and computes its representation as f(x) = v + Ax.
	  @param Matrix<Rational> rays The rays of the cone, always with leading coordinate.
	  @param Matrix<Rational> linspace The lineality space of the cone
	  @param Matrix<Rational> ray_values The values of the function on the rays (as row vectors) without leading coordinate.
	  @param Matrix<Rational> lin_values The values of the function on the lineality space (as row vectors) 
	  @param Vector<Rational> translate This will be set to be the constant translate v of the function 
	  (without leading coordinate).
	  @param Matrix<Rational> matrix This will be set to be the function matrix A (without leading coordinate).
	  */
	void computeConeFunction(	const Matrix<Rational> &rays, 
									 	const Matrix<Rational> &linspace,  
										const Matrix<Rational> &ray_values, 
										const Matrix<Rational> &lin_values, 
										Vector<Rational> &translate, Matrix<Rational> &matrix);

	/**
	  @brief Convenience function: Does the same as computeConeFunction for arbitrary functions, but the input values 
	  are specifically designed for rational functions, i.e. functions to R
	  @param Matrix<Rational> rays The rays of the cone
	  @param Matrix<Rational> linspace The lineality space of the cone
	  @param Vector<Rational> ray_values The values of the function on the rays 
	  @param Vector<Rational> lin_values The values of the function on the lineality space 
	  @param Rational translate This will be set to be the constant translate v of the function 
	  @param Vector<Rational> functional This will be set to be the function matrix A 
	  (which in this case is just a vector, since the function maps to R)
	  */
	void computeConeFunction(	const Matrix<Rational> &rays, 
										const Matrix<Rational> &linspace, 
										const Vector<Rational> &ray_values, 
										const Vector<Rational> &lin_values, 
										Rational &translate, Vector<Rational> &functional);

}}

#endif
