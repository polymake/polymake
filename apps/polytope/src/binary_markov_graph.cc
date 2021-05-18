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
#include "polymake/Rational.h"
#include "polymake/Graph.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Array.h"
#include "polymake/Vector.h"
#include "polymake/Matrix.h"
#include "polymake/linalg.h"

namespace polymake { namespace polytope {

BigObject binary_markov_graph(const Array<bool>& observation)
{
   const Vector<Rational> zero(2);
   const Vector<Rational> x1 = unit_vector<Rational>(2,0);
   const Vector<Rational> y1 = unit_vector<Rational>(2,1);
   Vector<Rational> x2(2), y2(2); x2[0] = y2[1] = 2;

   const Int n = observation.size();
   Graph<Directed> G(2*n+2); // unique source=0, sink=2*n+1
   EdgeMap<Directed, Vector<Rational> > Trans(G);

   Array<bool>::const_iterator ob = observation.begin();
   if (*ob) {
      Trans(0,1) = zero;
      Trans(0,2) = y1;
   } else {
      Trans(0,1) = x1;
      Trans(0,2) = zero;
   }
      
   for (Int i = 1; i < n; ++i)
      if (*(++ob)) {
         Trans(2*i-1, 2*i+1) = x1;
         Trans(2*i-1, 2*i+2) = y1;
         Trans(2*i, 2*i+1) = zero;
         Trans(2*i, 2*i+2) = y2;
      } else {
         Trans(2*i-1, 2*i+1) = x2;
         Trans(2*i-1, 2*i+2) = zero;
         Trans(2*i, 2*i+1) = x1;
         Trans(2*i, 2*i+2) = y1;
      }

   Trans(2*n-1,2*n+1) = zero;
   Trans(2*n,2*n+1) = zero;

   BigObject p("PropagatedPolytope",
               "SUM_PRODUCT_GRAPH.ADJACENCY", G,
               "SUM_PRODUCT_GRAPH.TRANSLATIONS", Trans,
               "CONE_AMBIENT_DIM", 3);
   p.set_description() << "Propagated polytope defined by the (maybe) simplest possible (non-trivial) Hidden Markov Model; "
                       << "see Joswig: Polytope Propagation on Graphs, Example 6.9." << endl;
   return p;
}

UserFunction4perl("# @category Producing a polytope from scratch"
                  "# Defines a very simple graph for a polytope propagation related to a Hidden Markov Model."
                  "# The propagated polytope is always a polygon."
                  "# For a detailed description see"
                  "#\t M. Joswig: Polytope propagation, in: Algebraic statistics and computational biology"
                  "#\t by L. Pachter and B. Sturmfels (eds.), Cambridge, 2005."
                  "# @param Array<Bool> observation"
                  "# @return PropagatedPolytope",
                  &binary_markov_graph, "binary_markov_graph(Array)");

InsertEmbeddedRule("# @category Producing a polytope from scratch"
                   "# @param String observation encoded as a string of \"0\" and \"1\".\n"
                   "user_function binary_markov_graph($) {\n"
                   "   binary_markov_graph(new Array<Bool>(is_string($_[0]) ? split //, $_[0] : $_[0]));\n"
                   "}\n");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
