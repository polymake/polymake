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
#include "polymake/FacetList.h"
#include "polymake/fan/tight_span.h"
#include "polymake/fan/hasse_diagram.h"

namespace polymake { namespace fan {

using namespace graph;
using namespace graph::lattice;
using namespace fan::lattice;

BigObject tight_span_lattice_for_subdivision(const IncidenceMatrix<>& maximal_cones,
                                             const Array<IncidenceMatrix<>>& maximal_vifs,
                                             const Int dim)
{
  // Compute boundary facets
  const Array<Int> max_dim_dummy;
  Lattice<BasicDecoration> top_lattice = hasse_diagram_general(maximal_cones, maximal_vifs,
      dim, max_dim_dummy, RankRestriction(true, RankCutType::GreaterEqual, dim),
      TopologicalType(1, 0), Set<Int>{});
  std::list<Set<Int>> max_boundary_faces;
  for (const auto mf : top_lattice.nodes_of_rank(dim)) {
    if (top_lattice.out_adjacent_nodes(mf).size() < 2)
      max_boundary_faces.push_back(top_lattice.face(mf));
  }
  NoBoundaryCut cut(max_boundary_faces, maximal_cones);
  BasicClosureOperator<> cop(maximal_cones.rows(), T(maximal_cones));
  BasicDecorator<> dec(0, scalar2set(-1));
  return static_cast<BigObject>(
           lattice_builder::compute_lattice_from_closure<BasicDecoration>(cop, cut, dec, 1, lattice_builder::Primal()));
}

Function4perl(&tight_span_lattice_for_subdivision,"tight_span_lattice_for_subdivision(IncidenceMatrix,Array<IncidenceMatrix>, $)");

FunctionTemplate4perl("tight_span_vertices<Scalar>(Matrix<Scalar>, IncidenceMatrix, Vector<Scalar>)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
