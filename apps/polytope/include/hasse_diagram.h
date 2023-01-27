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

#pragma once

#include "polymake/client.h"
#include "polymake/graph/Decoration.h"
#include "polymake/graph/Lattice.h"
#include "polymake/graph/BasicLatticeTypes.h"
#include "polymake/graph/lattice_builder.h"
#include "polymake/graph/LatticePermutation.h"

namespace polymake { namespace polytope {

using graph::Lattice;
using graph::lattice::BasicDecorator;
using graph::lattice::BasicDecoration;
using graph::lattice::BasicClosureOperator;    
using graph::lattice::Sequential;
using graph::lattice::Nonsequential;

// Helper function to carry out lattice computation
template<typename CutType, typename PrimalOrDual>      
Lattice<BasicDecoration, Sequential>
hasse_diagram_impl(BasicClosureOperator<>& cop,
                   CutType& cut,
                   BasicDecorator<>& dec,
                   PrimalOrDual primal_or_dual,
		   const IncidenceMatrix<>& VIF)
{
   Lattice<BasicDecoration, Sequential> init_lattice;
   Lattice<BasicDecoration, Sequential> result(graph::lattice_builder::compute_lattice_from_closure<BasicDecoration>(
                                                                                                                     cop, cut, dec, 0, primal_or_dual, init_lattice));
   sort_vertices_and_facets(result, VIF);
   return result;
}

// Compute full Hasse diagram of a cone
BigObject hasse_diagram(const IncidenceMatrix<>& VIF, Int cone_dim);

//Compute Hasse diagram of bounded faces of a polytope (possibly up to a dimension)
Lattice<BasicDecoration, Nonsequential> bounded_hasse_diagram_computation(
  const IncidenceMatrix<>& VIF,
  const Set<Int>& far_face,
  const Int boundary_dim = -1);

BigObject bounded_hasse_diagram(const IncidenceMatrix<>& VIF,
                                const Set<Int>& far_face,
                                const Int boundary_dim = -1);

BigObject rank_bounded_hasse_diagram(const IncidenceMatrix<>& VIF,
                                     Int cone_dim, Int boundary_dim, bool from_above);

inline
BigObject lower_hasse_diagram(const IncidenceMatrix<>& VIF, Int boundary_dim)
{
  return rank_bounded_hasse_diagram(VIF, 0,boundary_dim, false);
}

inline
BigObject upper_hasse_diagram(const IncidenceMatrix<>& VIF, Int cone_dim, Int boundary_dim)
{
  return rank_bounded_hasse_diagram(VIF, cone_dim, boundary_dim, true);
}

} }

