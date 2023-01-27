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
#include "polymake/Graph.h"
#include "polymake/Map.h"
#include "polymake/Set.h"
#include "polymake/PowerSet.h"

namespace polymake { namespace graph {

// the family of Graphs J(n,k,i) from [Godsil & Royle - algebraic graph theory]
// with nodes [n] choose k and an edge between two nodes if there intersection has size i.  
// i=0 gives the Kneser Graph and i=k-1 the Johnson Graph.
BigObject generalized_johnson_graph (const Int n, const Int k, const Int i)
{
   if (n < 1)
      throw std::runtime_error("generalized_johnson_graph: n should be positiv");
   if (k < 1 || k > n)
      throw std::runtime_error("generalized_johnson_graph: 1 <= k <= n required");

   Map<Set<Int>, Int> index_of;
   Array<std::string> labels(Int(Integer::binom(n, k)));

   Int ct = 0;
   auto lit = entire(labels);
   std::ostringstream os;
   for (auto sit = entire(all_subsets_of_k(sequence(0,n), k)); !sit.at_end(); ++sit, ++lit) {
      const Set<Int> the_set(*sit);
      index_of[the_set] = ct++;
      wrap(os) << the_set;
      *lit = os.str();
      os.str("");
   }

   Graph<> jgraph(ct);

   for (auto mit1 = entire(index_of); !mit1.at_end(); ++mit1)
      for (auto mit2 = mit1; !mit2.at_end(); ++mit2)
         if ((mit1->first * mit2->first).size()==i)
	    jgraph.edge(mit1->second, mit2->second);

   return BigObject("Graph<Undirected>",
                    "ADJACENCY", jgraph,
                    "N_NODES", ct,
                    "N_EDGES", jgraph.edges(),
                    "NODE_LABELS", labels);
}

BigObject kneser_graph(const Int n, const Int k)
{
   return generalized_johnson_graph(n, k, 0);
}

BigObject johnson_graph(const Int n, const Int k)
{
   return generalized_johnson_graph(n, k, k-1);
}

UserFunction4perl("# @category Producing a graph"
                  "# Create the __generalized Johnson graph__ on parameters (n,k,i)."
                  "#   It has one node for each set in \\({[n]}\\choose{k}\\),"
                  "#   and an edge between two nodes iff the intersection of the corresponding subsets is of size i."
                  "# @param Int n the size of the ground set"
                  "# @param Int k the size of the subsets"
                  "# @param Int i the size of the subsets"
                  "# @return Graph"
                  "# @example The following prints the adjacency representation of the generalized"
                  "# johnson graph with the parameters 4,2,1:"
                  "# > print generalized_johnson_graph(4,2,1)->ADJACENCY;"
                  "# | {1 2 3 4}"
                  "# | {0 2 3 5}"
                  "# | {0 1 4 5}"
                  "# | {0 1 4 5}"
                  "# | {0 2 3 5}"
                  "# | {1 2 3 4}",
                  &generalized_johnson_graph, "generalized_johnson_graph($$$)");

UserFunction4perl("# @category Producing a graph"
                  "# Create the __Kneser graph__ on parameters (n,k)."
                  "#   It has one node for each set in \\({[n]}\\choose{k}\\),"
                  "#   and an edge between two nodes iff the corresponding subsets are disjoint."
                  "# @param Int n the size of the ground set"
                  "# @param Int k the size of the subsets"
                  "# @return Graph"
                  "# @example The following prints the adjacency representation of the kneser"
                  "# graph with the parameters 3,1:"
                  "# > print kneser_graph(3,1)->ADJACENCY;"
                  "# | {1 2}"
                  "# | {0 2}"
                  "# | {0 1}",
                  &kneser_graph, "kneser_graph($$)");


UserFunction4perl("# @category Producing a graph"
                  "# Create the __Johnson graph__ on parameters (n,k)."
                  "#   It has one node for each set in \\({[n]}\\choose{k}\\),"
                  "#   and an edge between two nodes iff the intersection of the corresponding subsets is of size k-1."
                  "# @param Int n the size of the ground set"
                  "# @param Int k the size of the subsets"
                  "# @return Graph"
                  "# @example The following prints the adjacency representation of the johnson"
                  "# graph with the parameters 4,3:"
                  "# > print johnson_graph(4,3)->ADJACENCY;"
                  "# | {1 2 3}"
                  "# | {0 2 3}"
                  "# | {0 1 3}"
                  "# | {0 1 2}",
                  &johnson_graph, "johnson_graph($$)");

} }


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
