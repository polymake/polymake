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

	Computes a matroid fan in its chains-of-flats structure.
	*/

#include "polymake/client.h"
#include "polymake/Matrix.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Rational.h"
#include "polymake/Vector.h"
#include "polymake/Set.h"
#include "polymake/graph/Lattice.h"
#include "polymake/graph/Decoration.h"
#include "polymake/graph/maximal_chains.h"
#include "polymake/tropical/specialcycles.h"

namespace polymake { namespace tropical {

using graph::Lattice;
using graph::lattice::Sequential;
using graph::lattice::BasicDecoration;

template <typename Addition>
BigObject matroid_fan_from_flats(BigObject matroid)
{
  // Extract properties
  const Int n = matroid.give("N_ELEMENTS");
  const Set<Int> loops = matroid.give("LOOPS");
  if (loops.size() != 0) {
    return empty_cycle<Addition>(n-1);
  }
  BigObject flats_obj = matroid.give("LATTICE_OF_FLATS");
  Lattice<BasicDecoration, Sequential> flats(flats_obj);
  IncidenceMatrix<> chains_incidence( maximal_chains( flats, false,false));
  const IncidenceMatrix<> faces = flats_obj.give("FACES");

  const Int empty_index = flats_obj.give("BOTTOM_NODE");
  const Int top_index = flats_obj.give("TOP_NODE");

  // Create rays
  const Matrix<Rational> unitm = zero_vector<Rational>() | unit_matrix<Rational>(n);
  Matrix<Rational> rays(faces.rows(), n+1);
  rays(empty_index,0) = 1;
  for (Int f = 1; f < faces.rows()-1; ++f) {
    rays.row(f) = Addition::orientation() * accumulate(rows(unitm.minor(faces.row(f),All)), operations::add());
  }
  const auto extreme_nodes = scalar2set(top_index);
  chains_incidence = chains_incidence.minor(All, ~extreme_nodes);
  rays = rays.minor(~extreme_nodes, All);

  return BigObject("Cycle", mlist<Addition>(),
                   "PROJECTIVE_VERTICES", rays,
                   "MAXIMAL_POLYTOPES", chains_incidence,
                   "WEIGHTS", ones_vector<Integer>(chains_incidence.rows()));
}

UserFunctionTemplate4perl("# @category Matroids"
                          "# Computes the fan of a matroid in its chains-of-flats subdivision."
                          "# Note that this is potentially very slow for large matroids."
                          "# @param matroid::Matroid A matroid. Should be loopfree."
                          "# @tparam Addition Min or max, determines the matroid fan coordinates."
                          "# @return Cycle<Addition>",
                          "matroid_fan_from_flats<Addition>(matroid::Matroid)");
} }
