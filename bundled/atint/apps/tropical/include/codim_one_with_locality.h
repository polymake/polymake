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
	Copyright (c) 2016-2022
	Ewgenij Gawrilow, Michael Joswig, and the polymake team
	Technische Universität Berlin, Germany
	https://polymake.org


	We need specialized functions to compute codimension one data due to 
	locality and needing to do this without calls to perl side functionality.

*/

#pragma once

#include "polymake/IncidenceMatrix.h"
#include "polymake/Matrix.h"
#include "polymake/Rational.h"
#include "polymake/Map.h"

namespace polymake { namespace tropical {


	struct CodimensionOneResult {
		IncidenceMatrix<> codimOneCones;
		IncidenceMatrix<> codimOneInMaximal;
	};

	/**
	  @brief Computes the codimension one data of a polyhedral complex
	  @param Matrix<Rational> rays The ray matrix of the complex, non-tropical homog, but with leading coordinate.
	  @param IncidenceMatrix<> maximalCones The incidence matrix of the maximal cells
	  @param Matrix<Rational> linspace The generators of the lineality space
	  @param IncidenceMatrix<> local_restriction The local restriction of the complex
	  @return CodimensionOneResult A struct containing:
	  1) An IncidenceMatrix<> called codimOneCones describing the codimension one cells in terms of the rays
	  2) An IncidenceMatrix<> called codimOneInMaximal describing which codim one cell lies in which maximal cells
	  */
	CodimensionOneResult calculateCodimOneData(const Matrix<Rational> &rays, const IncidenceMatrix<> &maximalCones, const Matrix<Rational> &linspace, const IncidenceMatrix<>  &local_restriction);

}}

