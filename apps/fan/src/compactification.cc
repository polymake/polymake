/* Copyright (c) 1997-2020
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
#include "polymake/Rational.h"
#include "polymake/fan/compactification.h"
#include "polymake/graph/lattice_builder.h"
#include "polymake/graph/BasicLatticeTypes.h"


namespace polymake { namespace fan {
namespace compactification {

using SD=SedentarityDecoration;

BigObject compactify(BigObject pc) {
   CellularClosureOperator<SD, Rational> cco(pc);
   SedentarityDecorator sd(cco.get_int2vertices(), cco.get_farVertices());
   Lattice<SD, lattice::Nonsequential> hasseDiagram = graph::lattice_builder::compute_lattice_from_closure<SD>(cco, TrivialCut<SD>(), sd, true, std::false_type());
   return static_cast<BigObject>(hasseDiagram);
}

} // namespace compactification
      
Function4perl(&compactification::compactify, "compactify( $ )");
      
} // namespace fan
} // namespace polymake

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
  
