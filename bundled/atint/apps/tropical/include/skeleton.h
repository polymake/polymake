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
	Copyright (c) 2016-2023
	Ewgenij Gawrilow, Michael Joswig, and the polymake team
	Technische Universität Berlin, Germany
	https://polymake.org

	Computes the skeleton of a polyhedral complex
	*/

#pragma once

#include "polymake/client.h"
#include "polymake/Rational.h"
#include "polymake/Vector.h"
#include "polymake/Matrix.h"
#include "polymake/Set.h"
#include "polymake/tropical/codim_one_with_locality.h"
#include "polymake/tropical/specialcycles.h"
#include "polymake/tropical/thomog.h"

namespace polymake { namespace tropical {

/**
   @brief Takes a polyhedral complex and computes the k-skeleton. Will return an empty fan, if k is larger then the dimension of the given complex or smaller than 1.
   @param Cycle<Addition> fan A polyhedral complex
   @param Int k The dimension of the skeleton that should be computed
   @param Bool preserveRays When true, the function assumes that all rays of the fan remain in the k-skeleton, so it just copies the VERTICES, instead of computing a non-redundant list. By default, this property is false.
   @return The k-skeleton of the cycle 
*/
template <typename Addition>
BigObject skeleton_complex(BigObject complex, Int k, bool preserve = false)
{
  // Extract properties
  Int cmplx_dim = complex.give("PROJECTIVE_DIM");
  Int ambient_dim = complex.give("PROJECTIVE_AMBIENT_DIM");
  Matrix<Rational> rays = complex.give("VERTICES");
  rays = tdehomog(rays);
  IncidenceMatrix<> maximalCones = complex.give("MAXIMAL_POLYTOPES");
  Matrix<Rational> lineality = complex.give("LINEALITY_SPACE");
  lineality = tdehomog(lineality);
  Int lineality_dim = complex.give("LINEALITY_DIM");
  IncidenceMatrix<> local_restriction;
  complex.lookup("LOCAL_RESTRICTION") >> local_restriction;

  // If the skeleton dimension is too small, return the 0-cycle
  if (k < 0 || k < lineality_dim) {
    return empty_cycle<Addition>(ambient_dim);
  }

  // If the skeleton dimension is the fans dimension, return the fan
  if (k == cmplx_dim) {
    return complex;
  }

  Vector<Set<Int>> new_local_restriction;

  // Now we compute the codimension one skeleton of the fan (cmplx_dim - k) times 
  IncidenceMatrix<> newMaximalCones = maximalCones;
  for (Int i = 1; i <= (cmplx_dim - k); i++) {
    newMaximalCones = calculateCodimOneData(rays, newMaximalCones, lineality, local_restriction).codimOneCones;
  }

  // Now return the result - made irredundant, if preserve is false
  Matrix<Rational> newrays = rays;
  if (!preserve) {
    // Take the union of all cones to see what rays are used
    Set<Int> usedRays;
    for (Int c = 0; c < newMaximalCones.rows(); ++c) {
      usedRays += newMaximalCones.row(c);
    }

    if (local_restriction.rows() > 0) {
      Map<Int, Int> index_map; //Maps indices of old rays to indices of new rays
      Int newIndex = 0;
      for (auto uR = entire(usedRays); !uR.at_end(); ++uR) {
        index_map[*uR] = newIndex;
        ++newIndex;
      }
      for (Int i = 0; i < local_restriction.rows(); ++i) {
        new_local_restriction |= attach_operation(local_restriction.row(i) * usedRays, pm::operations::associative_access<Map<Int, Int>, Int>(&index_map));
      }
    }

    newrays = newrays.minor(usedRays,All);
    newMaximalCones = newMaximalCones.minor(All,usedRays);
  }

  BigObject result("Cycle", mlist<Addition>());
  result.take("VERTICES") << thomog(newrays);
  result.take("MAXIMAL_POLYTOPES") << newMaximalCones;
  result.take("LINEALITY_SPACE") << thomog(lineality);
  if (local_restriction.rows() > 0)
    result.take("LOCAL_RESTRICTION") << new_local_restriction;

  return result;
}

} }

