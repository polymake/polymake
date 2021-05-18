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

*/

#pragma once

#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/linalg.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Set.h"
#include "polymake/tropical/convex_hull_tools.h"

namespace polymake { namespace tropical {
   
/**
   @brief Computes the Star around a point in a given set of cones
   @param point The point around which the star is to be computed, given in non-trop.-homog. coordinates
   @param rays The rays of the surrounding cones (needn't contain point) in non-trop.-homog. coordinates
   @param cones The surrounding cones (i.e. should all contain point). Should form a complex.
   @param result_rays Will contain the rays of the result (without leading zeros). Might not be irredundant
   @param result_cones Will contain the cones of the result. A cone might be given by a redundant list of rays. The cones will be in the exact same order as in "cones", i.e. the i-th fan cone is the star of the i-th cone at point. Note that the rays are not in any way irredundant (but we don't care for this computation)
*/
template <typename TConesMatrix>
std::pair<Matrix<Rational>, std::vector<Set<Int>>>
computeStar(const Vector<Rational>& point,
            const Matrix<Rational>& rays, 
            const GenericIncidenceMatrix<TConesMatrix>& cones)
{
  // Prepare result variables
  ListMatrix<Vector<Rational>> result_rays(0, rays.cols()-1);
  std::vector<Set<Int>> result_cones;

  //Iterate all surrounding cones
  for (auto sc = entire(rows(cones)); !sc.at_end(); ++sc) {
    Set<Int> cone;
    for (auto r = entire(*sc); !r.at_end(); ++r) {
      if (rays(*r, 0) == 0) {
        result_rays /= rays.row(*r).slice(range_from(1));
      } else {
        result_rays /= rays.row(*r).slice(range_from(1)) - point.slice(range_from(1));
      }
      cone += (result_rays.rows()-1);
    }
    result_cones.push_back(cone);
  }

  normalize_rays(result_rays);
  return { Matrix<Rational>(result_rays), result_cones };
}

} }

