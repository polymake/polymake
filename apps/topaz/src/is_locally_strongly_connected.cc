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
#include "polymake/topaz/complex_tools.h"
#include "polymake/topaz/graph.h"
#include "polymake/graph/connected.h"

namespace polymake { namespace topaz {

bool is_locally_strongly_connected(BigObject p, OptionSet options)
{
   const Lattice<BasicDecoration>& HD = p.give("HASSE_DIAGRAM");
   const Int dim = HD.rank()-2;

   const bool verbose=options["verbose"],
                  all=options["all"];
   bool loc_str_conn = true;

   for (Int d = 0; d < dim; ++d)
      for (const auto f : HD.nodes_of_rank(d+1)) {
         const FacetList star(star_in_HD(HD, f));

         if (!graph::is_connected(dual_graph(star))) {
            if (verbose)
               cout << "is_locally_strongly_connected: star(" << HD.face(f) << ") is not strongly connected." << endl;
            loc_str_conn = false;
            if (!all) break;
         }
      }

   return loc_str_conn;
}

Function4perl(&is_locally_strongly_connected, "is_locally_strongly_connected(SimplicialComplex { verbose=>0, all=>0 })");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
