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

	Implements the computation of a recession fan.
	*/


#include "polymake/client.h"
#include "polymake/Rational.h"
#include "polymake/Set.h"
#include "polymake/Matrix.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Vector.h"
#include "polymake/tropical/misc_tools.h"
#include "polymake/tropical/make_complex.h"
#include "polymake/linalg.h"

namespace polymake { namespace tropical {

// Documentation see perl wrapper
template <typename Addition>
BigObject recession_fan(BigObject complex)
{
  // Special cases
  if (call_function("is_empty", complex))
    return complex;

  // Extract values
  Matrix<Rational> rays = complex.give("VERTICES");
  IncidenceMatrix<> cones = complex.give("MAXIMAL_POLYTOPES");
  Matrix<Rational> linspace = complex.give("LINEALITY_SPACE");
  Vector<Integer> weights = complex.give("WEIGHTS");
  Int dim = complex.give("PROJECTIVE_DIM");

  const std::pair<Set<Int>, Set<Int>> sorted_vertices = far_and_nonfar_vertices(rays);
  const Set<Int>& directional = sorted_vertices.first;

  // If there is only one vertex, shift it.
  if (directional.size() == rays.rows()-1) {
    Int vertex = sorted_vertices.second.front();
    rays.row(vertex) = unit_vector<Rational>(rays.cols(), 0);
    return BigObject("Cycle", mlist<Addition>(),
                     "PROJECTIVE_VERTICES", rays,
                     "MAXIMAL_POLYTOPES", cones,
                     "WEIGHTS", weights);
  }

  Matrix<Rational> newrays = rays.minor(directional,All);
  Matrix<Rational> newlineality = linspace;
  Vector<Integer> newweights;

  // Re-map ray indices
  Map<Int, Int> indexMap;
  Int i = 0;
  for (auto d = entire(directional); !d.at_end(); ++d, ++i) {
    indexMap[*d] = i;
  }

  // We compute the recession cone of each cell
  Vector<Set<Int>> rec_cones;
  for (Int mc = 0; mc < cones.rows(); ++mc) {
    Set<Int> mcDirectional = directional * cones.row(mc);
    // Compute that it has the right dimension
    const Int mcDim = rank(rays.minor(mcDirectional,All));
    if (mcDirectional.size() > 0 && mcDim == dim) {
      Set<Int> transformCone{ indexMap.map(mcDirectional) };
      rec_cones |= transformCone;
      newweights |= weights[mc];
    }
  }

  // Add vertex
  newrays /= unit_vector<Rational>(newrays.cols(),0);
  for (Int mc = 0; mc < rec_cones.dim(); ++mc) {
    rec_cones[mc] += scalar2set(newrays.rows()-1);
  }

  // Compute the complexification of the recession cones
  BigObject cplxify = make_complex<Addition>(newrays,rec_cones,newweights);
  // Extract its values and put them into the result
  BigObject result("Cycle", mlist<Addition>(),
                   "VERTICES", cplxify.give("VERTICES"),
                   "MAXIMAL_POLYTOPES", cplxify.give("MAXIMAL_POLYTOPES"),
                   "WEIGHTS", cplxify.give("WEIGHTS"),
                   "LINEALITY_SPACE", newlineality);
  result.set_description() << "Recession fan of \"" << complex.description() << "\"";
  return result;
}

UserFunctionTemplate4perl("# @category Basic polyhedral operations"
                          "# Computes the recession fan of a tropical variety. WARNING: This is a highly experimental"
                          "# function. If it works at all, it is likely to take a very long time for larger objects."
                          "# @param Cycle complex A tropical variety"
                          "# @return Cycle A tropical fan, the recession fan of the complex",
                          "recession_fan<Addition>(Cycle<Addition>)");
} }
