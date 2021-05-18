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

	Defines a function to compute the minimal interior cells of a polyhedral subdivision.
	*/

#pragma once

#include "polymake/Matrix.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Rational.h"

namespace polymake { namespace tropical{

/*
 * @brief Computes the minimal interior cells of a polyhedral subdivision of a polyhedron.
 * @param Matrix<Rational> vertices The vertices of the subdivision.
 * @param IncidenceMatrix<> polytopes The subdivision cells 
 * @return IncidenceMatrix<> The minimal interior cells of the subdivision.
 */
IncidenceMatrix<> minimal_interior(const Matrix<Rational> &vertices, 
                                   const IncidenceMatrix<>& polytopes);


/*
 * @brief This takes a cycle with nontrivial [[LOCAL_RESTRICTION]] and another cycle refining the first one
 * and computes the correct [[LOCAL_RESTRICTION]] for the second cycle
 * @param Cycle<Addition> localized_cycle
 * @param Cycle<Addition> refining_cycle
 * @return IncidenceMatrix<>
 */
IncidenceMatrix<> refined_local_cones(BigObject localized_cycle, BigObject refining_cycle);

} }

