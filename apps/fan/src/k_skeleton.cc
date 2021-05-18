/* Copyright (c) 1997-2021
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

/*
#include "polymake/PowerSet.h"
#include "polymake/graph/Closure.h"
#include "polymake/graph/Lattice.h"
#include "polymake/graph/BasicLatticeTypes.h"
#include "polymake/graph/lattice_builder.h"
*/

namespace polymake { namespace fan { 

template <typename Coord>
BigObject k_skeleton(BigObject fan, const Int k)
{
  const bool is_pure = fan.give("PURE");
  const bool is_complete = fan.give("COMPLETE");
  Matrix<Coord> rays = fan.give("RAYS");
  BigObject hasseDiagram = lower_hasse_diagram(fan, k, is_pure, is_complete);
  return BigObject("PolyhedralFan", mlist<Coord>(),
                   "RAYS", rays,
                   "HASSE_DIAGRAM", hasseDiagram);
}

UserFunctionTemplate4perl("# @category Producing a fan"
                          "# Computes the //k//-skeleton of the polyhedral fan //F//,"
                          "# i.e. the subfan of //F// consisting of all cones of dimension <=//k//."
                          "# @tparam Coord"
                          "# @param PolyhedralFan F"
                          "# @param Int k the desired top dimension"
                          "# @return PolyhedralFan",
                          "k_skeleton<Coord>(fan::PolyhedralFan<Coord>, $)");
   

} // namespace fan
} // namespace polymake
