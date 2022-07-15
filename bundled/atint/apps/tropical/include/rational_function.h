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

	Functions for the basic ruleset of RationalFunction
	*/

#include "polymake/client.h"
#include "polymake/Set.h"
#include "polymake/Array.h"
#include "polymake/Matrix.h"
#include "polymake/ListMatrix.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Vector.h"
#include "polymake/Rational.h"
#include "polymake/Polynomial.h"
#include "polymake/TropicalNumber.h"
#include "polymake/common/incidence_tools.h"
#include "polymake/tropical/specialcycles.h"
#include "polymake/tropical/refine.h"
#include "polymake/tropical/misc_tools.h"
#include "polymake/tropical/linear_algebra_tools.h"
#include "polymake/tropical/polynomial_tools.h"

#pragma once

namespace polymake { namespace tropical {

template <typename Addition>
BigObject computePolynomialDomain(const Polynomial<TropicalNumber<Addition>>& p)
{
  Matrix<Rational> monoms(p.monomials_as_matrix());
  Vector<TropicalNumber<Addition>> coefs = p.coefficients_as_vector();

  if (monoms.rows() <= 1) {
    return projective_torus<Addition>(monoms.cols()-1,0);
  }

  // FIXME Same computation as in the beginning of the hypersurface client. Refactor?

  // We have to make all exponents positive, otherwise the below equations produce
  // a wrong result. We multiply the polynomial with a single monomial, which 
  // does not change the hypersurface.
  Vector<Rational> min_degrees(monoms.cols());
  for (Int v = 0; v < monoms.cols(); ++v) {
    min_degrees[v] = accumulate(monoms.col(v),operations::min());
    // If the minimal degree is positive, we're good
    min_degrees[v] = std::min(min_degrees[v],Rational(0));
  }
  for (Int m = 0; m < monoms.rows(); ++m) {
    monoms.row(m) -= min_degrees; 
  }

  ListMatrix<Vector<Rational>> ineq;
  const TropicalNumber<Addition> zero=TropicalNumber<Addition>::zero();
  for (Int i=0; i< monoms.rows(); ++i) {
    if (coefs[i]==zero)
      ineq /= unit_vector<Rational>(monoms.cols()+1,0);
    else
      ineq /= Addition::orientation()*(Rational(coefs[i])|monoms[i]);
  }

  BigObject dome("polytope::Polytope<Rational>",
                 "INEQUALITIES", ineq,
                 "FEASIBLE", true,
                 "BOUNDED", false);

  const Matrix<Rational> vertices = dome.give("VERTICES");
  const Matrix<Rational> lineality = dome.give("LINEALITY_SPACE");
  IncidenceMatrix<> polytopes = dome.give("VERTICES_IN_FACETS");

  // Find and eliminate the far face
  const Set<Int> far_face = dome.give("FAR_FACE");
  const Int r = common::find_row(polytopes, far_face);
  if (r >= 0)
    polytopes = polytopes.minor(~scalar2set(r), All);

  return BigObject("Cycle", mlist<Addition>(),
                   "PROJECTIVE_VERTICES", vertices,
                   "MAXIMAL_POLYTOPES", polytopes,
                   "LINEALITY_SPACE", lineality);
}

} }

