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

	Reconstructs the matroid from a bergman fan.
	*/


#include "polymake/client.h"
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/Array.h"
#include "polymake/Set.h"
#include "polymake/PowerSet.h"
#include "polymake/list"
#include "polymake/tropical/specialcycles.h"
#include "polymake/tropical/misc_tools.h"
#include "polymake/tropical/thomog.h"

namespace polymake { namespace tropical {

template <typename Addition>
BigObject matroid_from_fan(BigObject cycle)
{
  // Find rank and ground set
  Int ambient_dim = cycle.give("PROJECTIVE_AMBIENT_DIM");
  Int n = ambient_dim+1;
  Int dim = cycle.give("PROJECTIVE_DIM");
  Int r = dim+1;

  if (dim == ambient_dim) {
    return call_function("matroid::uniform_matroid", n, n);
  }

  // FIXME Testing this could be done in a more efficient way by
  // finding all cones containing the origin and testing for
  // complementarity with unit vectors.
  // Take all r-sets and check if they are a basis
  Array<Set<Int>> rsets{ all_subsets_of_k(sequence(0,n), r) };
  std::list<Set<Int>> bases;
  for (const auto& rset : rsets) {
    BigObject hp = affine_linear_space<Addition>(unit_matrix<Rational>(n).minor(~rset, All));
    BigObject inter = call_function("intersect", cycle, hp);
    bool empty = call_function("is_empty", inter);
    if (!empty) bases.push_back(rset);
  }
  return BigObject("matroid::Matroid",
                   "N_ELEMENTS", n,
                   "BASES", Array<Set<Int>>(bases));
}

UserFunctionTemplate4perl("# @category Matroids"
                          "# Takes the bergman fan of a matroid and reconstructs the corresponding matroid"
                          "# The fan has to be given in its actual matroid coordinates, not as an isomorphic"
                          "# transform. The actual subdivision is not relevant."
                          "# @param Cycle<Addition> A tropical cycle, the Bergman fan of a matroid"
                          "# @return matroid::Matroid",
                          "matroid_from_fan<Addition>(Cycle<Addition>)");
} }
