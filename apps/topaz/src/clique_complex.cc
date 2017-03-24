/* Copyright (c) 1997-2015
   Ewgenij Gawrilow, Michael Joswig (Technische Universitaet Berlin, Germany)
   http://www.polymake.org

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

#include <string>
#include "polymake/client.h"
#include "polymake/Array.h"
#include "polymake/PowerSet.h"
#include "polymake/vector"

namespace polymake { namespace topaz {

perl::Object clique_complex(perl::Object graph, perl::OptionSet options)
{
   const PowerSet<int> maxCliques = graph.give("MAX_CLIQUES");
   const bool no_labels = options["no_labels"];
   
   perl::Object complex("topaz::SimplicialComplex");
   complex.set_description() << "Clique complex of graph " << graph.name() << "." << endl;
   complex.take("FACETS") << as_array(maxCliques);
   
   if (!no_labels) {
     const int n_nodes=graph.give("N_NODES");
     std::vector<std::string> labels(n_nodes);
     read_labels(graph, "NODE_LABELS", labels);
     complex.take("VERTEX_LABELS") << labels;
   }
   return complex;
}

UserFunction4perl("# @category Producing a simplicial complex from other objects\n"
                  "# Produce the __clique complex__ of a given graph, that is, the simplicial"
                  "# complex that has an n-dimensional facet for each n+1-clique.\n"
                  "# If //no_labels// is set to 1, the labels are not copied.\n"
                  "# @param Graph graph"
                  "# @option Bool no_labels Do not create [[VERTEX_LABELS]]. default: 0"
                  "# @return SimplicialComplex"
                  "# @example Create the clique complex of a simple graph with one 3-clique and"
                  "#  one 2-clique, not creating labels."
                  "# > $g = graph_from_edges([[0,1],[0,2],[1,2],[2,3]]);"
                  "# > $c = clique_complex($g,no_labels=>1);"
                  "# > print $c->FACETS;"
                  "# | {0 1 2}"
                  "# | {2 3}",
                  &clique_complex,"clique_complex(Graph; { no_labels => 0 })");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
