/* Copyright (c) 1997-2019
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

#ifndef POLYMAKE_POLYTOPE_HASSE_DIAGRAM_H
#define POLYMAKE_POLYTOPE_HASSE_DIAGRAM_H

#include "polymake/client.h"
#include "polymake/graph/Decoration.h"
#include "polymake/graph/Lattice.h"
#include "polymake/graph/BasicLatticeTypes.h"
#include "polymake/graph/lattice_builder.h"
#include "polymake/graph/LatticePermutation.h"

namespace polymake { namespace polytope {

using graph::Lattice;
using graph::lattice::BasicDecoration;
using graph::lattice::Sequential;
using graph::lattice::Nonsequential;

// Compute full Hasse diagram of a cone
perl::Object hasse_diagram(const IncidenceMatrix<>& VIF, int cone_dim);

//Compute Hasse diagram of bounded faces of a polytope (possibly up to a dimension)
Lattice<BasicDecoration, Nonsequential> bounded_hasse_diagram_computation(
  const IncidenceMatrix<>& VIF,
  const Set<int>& far_face,
  const int boundary_dim = -1);

perl::Object bounded_hasse_diagram(const IncidenceMatrix<>& VIF,
                                   const Set<int>& far_face,
                                   const int boundary_dim = -1);

perl::Object rank_bounded_hasse_diagram(const IncidenceMatrix<>& VIF,
                                        int cone_dim, int boundary_dim, bool from_above);

inline
perl::Object lower_hasse_diagram(const IncidenceMatrix<>& VIF, int boundary_dim)
{
  return rank_bounded_hasse_diagram(VIF, 0,boundary_dim, false);
}

inline
perl::Object upper_hasse_diagram(const IncidenceMatrix<>& VIF, int cone_dim, int boundary_dim)
{
  return rank_bounded_hasse_diagram(VIF, cone_dim, boundary_dim, true);
}

} }

#endif
