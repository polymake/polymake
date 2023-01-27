/* Copyright (c) 1997-2023
   Ewgenij Gawrilow, Michael Joswig, and the polymake team
   Technische Universität Berlin, Germany
   https://polymake.org

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version: http://www.gnu.org/licenses/gpl.txt.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
--------------------------------------------------------------------------------
*/

#include "polymake/client.h"
#include "polymake/Vector.h"
#include "polymake/Matrix.h"
#include "polymake/Rational.h"
#include "polymake/Map.h"
#include "polymake/Array.h"
#include "polymake/Set.h"
#include "polymake/IncidenceMatrix.h"

namespace polymake { namespace tropical {

Array<Rational> cycle_edge_lengths(BigObject cycle)
{
  const Matrix<Rational>& vertices = cycle.give("VERTICES");
  const Set<Int>& far_vertices = cycle.give("FAR_VERTICES");
  const Map<std::pair<Int, Int>, Vector<Integer>>& lattice_normals = cycle.give("LATTICE_NORMALS");
  const IncidenceMatrix<>& maximal_polytopes = cycle.give("MAXIMAL_POLYTOPES");
  const IncidenceMatrix<>& codim_one_incidence = cycle.give("MAXIMAL_AT_CODIM_ONE");
  const auto& maximal_incidence = T(codim_one_incidence);

  Array<Rational> result(maximal_polytopes.rows());
  auto result_it = entire(result);
  auto incidence_it = entire(rows(maximal_incidence));
  Int mp_index = 0;
  for (auto mp = entire(rows(maximal_polytopes)); !mp.at_end(); 
            ++mp, ++mp_index, ++result_it, ++incidence_it) {
    if (! ((*mp) * far_vertices).empty()) {
      *result_it = std::numeric_limits<Rational>::infinity(); continue;
    }
    const Int adj_codim = *(incidence_it->begin());
    Vector<Integer> lnormal = lattice_normals[ std::make_pair(adj_codim, mp_index)];
    const Matrix<Rational> ends = vertices.minor( *mp, All);
    const Vector<Rational> diff = ends.row(0) - ends.row(1);
    auto lnormal_it = entire(lnormal);
    for (auto diff_it = entire(diff); !diff_it.at_end(); ++diff_it, ++lnormal_it) {
      if (*diff_it != 0) {
        *result_it = abs(*diff_it / *lnormal_it); break;
      }
    }
  }
  return result;
}

Function4perl(&cycle_edge_lengths, "cycle_edge_lengths(Cycle)");

} }
