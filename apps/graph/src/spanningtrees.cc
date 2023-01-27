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

#include <algorithm>
#include <random>
#include "polymake/RandomGenerators.h"
#include "polymake/RandomSubset.h"
#include "polymake/client.h"
#include "polymake/Array.h"
#include "polymake/graph/all_spanningtrees.h"
#include "polymake/group/action.h"


namespace polymake { namespace graph {

std::pair<Array<Set<Int>>, Array<std::pair<Int,Int>>> calc_all_spanningtrees(const Graph<>& G)
{
   return all_spanningtrees(G);
}


Array<std::pair<Int,Int>> random_spanningtree(const Graph<>& G, OptionSet options){
   const RandomSeed seed(options["seed"]);
   Array<Int> perm(G.nodes(), random_permutation(G.nodes(), seed).begin());
   Array<Int> inv_perm = group::inverse(perm);
   Graph<> Gperm(G.nodes());
   for(auto edge=entire(edges(G)); !edge.at_end(); ++edge){
      Gperm.add_edge(perm[edge.from_node()], perm[edge.to_node()]);
   }
   std::pair<Set<Int>, Array<std::pair<Int,Int>>> treeperm = initial_spanningtree(G);
   Array<std::pair<Int,Int>> result(treeperm.first.size());
   Int k = 0;
   for(const auto& i : treeperm.first){
      std::pair<Int,Int> edge = (treeperm.second)[i];
      result[k] = {inv_perm[edge.first], inv_perm[edge.second]};
      k++;
   }
   return result;
}

UserFunction4perl("# @category Combinatorics"
                  "# Return a random spanning tree of a graph"
                  "# @param Graph G being connected"
                  "# @return Array<Pair<Int,Int>> edges of spanning tree",
                  &random_spanningtree, "random_spanningtree($ {seed=>undef})");

UserFunction4perl("# @category Combinatorics"
                  "# Calculate all spanning trees for a connected graph along the lines of"
                  "#\t Donald E. Knuth: The Art of Computer Programming, Volume 4, Fascicle 4, 24-31, 2006, Pearson Education Inc."
                  "# Every spanning tree is represented as a set of indices of the edges used. The result is a pair"
                  "# of an array of the spanning trees and an array translating the indices used into actual edges,"
                  "# i.e. the i-th entry of the dictionary is a pair of integers representing the end nodes of the"
                  "# i-th edge."
                  "# @param Graph G being connected"
                  "# @return Pair<Array<Set<Int>>, Array<Pair<Int,Int>>>"
                  "# @example The following prints all spanning trees of the complete graph with"
                  "# 3 nodes, whereby each line represents a single spanning tree as an edge set:"
                  "# > print all_spanningtrees(complete(3)->ADJACENCY);"
                  "# | <{0 1}"
                  "# | {1 2}"
                  "# | {0 2}"
                  "# | >"
                  "# | (1 0) (2 0) (2 1)",
                  &calc_all_spanningtrees, "all_spanningtrees");

} }

// Local Variables:
// c-basic-offset:3
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
