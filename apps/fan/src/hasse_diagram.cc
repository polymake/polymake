/* Copyright (c) 1997-2022
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
#include "polymake/fan/hasse_diagram.h"

namespace polymake { namespace fan {

using namespace graph::lattice;
using namespace fan::lattice;

graph::Lattice<BasicDecoration> empty_fan_hasse_diagram()
{
   graph::Lattice<BasicDecoration> HD;
   const Int bottom_node = HD.add_node(BasicDecoration{ Set<Int>{}, 0 });
   const Int top_node = HD.add_node(BasicDecoration{Set<Int>{-1}, 1 });
   HD.add_edge(bottom_node, top_node);
   return HD;
}

BigObject hasse_diagram_caller(BigObject fan, const RankRestriction& rr, const TopologicalType& tt, 
                                  const Set<Int>& far_vertices = Set<Int>{})
{
   const IncidenceMatrix<>& maximal_cones = fan.give("MAXIMAL_CONES");
   Array<IncidenceMatrix<> > maximal_vifs;
   if (!tt.is_complete)
      fan.give("MAXIMAL_CONES_INCIDENCES") >> maximal_vifs;
   const Int dim = fan.give("COMBINATORIAL_DIM");
   Array<Int> maximal_dims;
   if (!tt.is_pure)
      fan.give("MAXIMAL_CONES_COMBINATORIAL_DIMS") >> maximal_dims;
   return static_cast<BigObject>(hasse_diagram_general(maximal_cones, maximal_vifs, dim, maximal_dims, rr, tt, far_vertices));
}

BigObject hasse_diagram(BigObject fan, bool is_pure, bool is_complete)
{
   return hasse_diagram_caller(fan, RankRestriction(), TopologicalType(is_pure, is_complete));
}

BigObject lower_hasse_diagram(BigObject fan, Int boundary_rank, bool is_pure, bool is_complete)
{
   return hasse_diagram_caller(fan, RankRestriction(true, RankCutType::LesserEqual, boundary_rank),
                               TopologicalType(is_pure, is_complete));
}

BigObject upper_hasse_diagram(BigObject fan, Int boundary_rank, bool is_pure, bool is_complete)
{
   return hasse_diagram_caller(fan, RankRestriction(true, RankCutType::GreaterEqual, boundary_rank),
                               TopologicalType(is_pure, is_complete));
}

BigObject bounded_hasse_diagram(BigObject complex, Int boundary_rank, bool is_pure)
{
   const Set<Int>& far_vertices = complex.give("FAR_VERTICES");
   return hasse_diagram_caller( complex, 
                                RankRestriction(boundary_rank >= 0, RankCutType::LesserEqual, boundary_rank),
                                TopologicalType(is_pure, false), far_vertices);
}

Function4perl(&hasse_diagram, "hasse_diagram(PolyhedralFan;$=0, $=0)");
Function4perl(&lower_hasse_diagram, "lower_hasse_diagram(PolyhedralFan, $;$=0, $=0)");
Function4perl(&upper_hasse_diagram, "upper_hasse_diagram(PolyhedralFan, $; $=0, $=0)");
Function4perl(&bounded_hasse_diagram, "bounded_hasse_diagram(PolyhedralComplex;$=-1,$=0)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
