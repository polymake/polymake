/* Copyright (c) 1997-2014
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

#include "polymake/client.h"
#include "polymake/Graph.h"
#include "polymake/Map.h"
#include "polymake/Set.h"
#include "polymake/PowerSet.h"

namespace polymake { namespace graph {

perl::Object kneser_graph (int n, int k) {
   perl::Object Kneser("Graph<Undirected>");
   Map<Set<int>, int> index_of;
   Array<std::string> labels(Integer::binom(n,k).to_int());

   int ct(0);
   Entire<Array<std::string> >::iterator lit = entire(labels);
   std::ostringstream os;
   for (Entire<Subsets_of_k<const sequence&> >::const_iterator sit = entire(all_subsets_of_k(sequence(0,n), k)); !sit.at_end(); ++sit, ++lit) {
      const Set<int> the_set(*sit);
      index_of[the_set] = ct++;
      wrap(os) << the_set;
      *lit = os.str();
      os.str("");
   }
      
   Graph<> kneser(ct);
      
   for(Entire<Map<Set<int>, int> >::const_iterator mit1 = entire(index_of); !mit1.at_end(); ++mit1) 
      for(Entire<Map<Set<int>, int> >::const_iterator mit2 = mit1; !mit2.at_end(); ++mit2) 
         if (!(mit1->first * mit2->first).size())
	    kneser.edge(mit1->second, mit2->second);

   Kneser.take("ADJACENCY") << kneser;
   Kneser.take("N_NODES") << ct;
   Kneser.take("N_EDGES") << kneser.edges();
   Kneser.take("NODE_LABELS") << labels;
   return Kneser;
}



UserFunction4perl("# @category Creating from scratch"
                  "# Create the Kneser graph on parameters (n,k)"
                  "#   It has one node for each set in binomial{[n]}{k},"
                  "#   and an edge between two nodes iff the corresponding subsets are disjoint"
                  "# @param Int n the size of the ground set"
                  "# @param Int k the size of the subsets",
                  &kneser_graph, "kneser_graph($$)");

} }


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
