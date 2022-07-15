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

	These functions wrap the basic convex hull functionality of ppl, taking into account
	tropical homogeneous coordinates

*/


#pragma once

#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/polytope/convex_hull.h"

namespace polymake { namespace tropical {

/*
 * @brief Computes the facets of a polyhedral cell in tropical homogeneous coordinates
 * This function will add a lineality space of (1,..,1) to the cone before computing facets
 */
inline
auto enumerate_homogeneous_facets(const Matrix<Rational>& rays,
                                  const Matrix<Rational>& lineality)
{
  const Int n = std::max(rays.cols(), lineality.cols());
  const auto one_lin = ones_vector<Rational>(n) - unit_vector<Rational>(n,0);
  return polytope::enumerate_facets(rays, lineality / one_lin, false);
}


/*
 * @brief Computes the vertices of a polyhedral cell in tropical homogeneous coordiantes.
 * Assumes that the facet description is such that (1,..,1) is actually part of the cell.
 * Will adjust the lineality space of the result such that (1,..,1) is NOT in it.
 */
inline
auto enumerate_homogeneous_vertices(const Matrix<Rational>& facets,
                                    const Matrix<Rational>& affine)
{
  auto p = polytope::try_enumerate_vertices(facets, affine, false);

  if (p.second.rows() > 0) {
    Int n = p.second.cols();
    const auto one_lin = ones_vector<Rational>(n) - unit_vector<Rational>(n,0);
    Matrix<Rational> kernel = null_space(T(p.second /one_lin));
    if (kernel.rows() == 0)
      throw std::runtime_error("Invalid tropical homogeneous facet description. Cell does not contain (1,..,1)-lineality space");
    for (Int c = 0; c < kernel.cols(); c++) {
      if (kernel(0,c) != 0) {
        p.second = p.second.minor(~scalar2set(c),All);
        break;
      }
    }
  }

  return p;
}

} }

