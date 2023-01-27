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
#include "polymake/graph/poset_tools.h"

namespace polymake { namespace graph {

using namespace poset_tools;

Array<Array<Int>>
poset_homomorphisms(const BigObject p, const BigObject q, OptionSet options)
{
   const Graph<Directed> 
      P = p.give("ADJACENCY"),
      Q = q.give("ADJACENCY");
   const Array<Int> prescribed_map = options["prescribed_map"];

   RecordKeeper<HomList> record_keeper;

   const HomList result(poset_homomorphisms_impl(P, Q, record_keeper, prescribed_map));
   return Array<Array<Int>>(result.size(), entire(result));
}

Int
n_poset_homomorphisms(const BigObject p, const BigObject q, OptionSet options)
{
   const Graph<Directed> 
      P = p.give("ADJACENCY"),
      Q = q.give("ADJACENCY");
   const Array<Int> prescribed_map = options["prescribed_map"];

   RecordKeeper<Int> record_keeper;

   return poset_homomorphisms_impl(P, Q, record_keeper, prescribed_map);
}
      
Graph<Directed> hom_poset_pq(const BigObject p, const BigObject q)
{
   const Graph<Directed> 
      P = p.give("ADJACENCY"),
      Q = q.give("ADJACENCY");
   return hom_poset_impl(P, Q);
}

Graph<Directed> hom_poset_hq(const Array<Array<Int>>& homs, const BigObject q)
{
   const Graph<Directed> Q = q.give("ADJACENCY");
   return hom_poset_impl(homs, Q);
}

Graph<Directed> covering_relations(const BigObject p)
{
   const Graph<Directed> P = p.give("ADJACENCY");
   return covering_relations_impl(P);
}

template <typename TSet, typename = std::enable_if_t<pm::is_generic_set<TSet>::value>>
Graph<Directed> poset_by_inclusion(const Array<TSet>& collection)
{
   const Int m = collection.size();
   Graph<Directed> poset(m);
   for (Int i = 0; i < m-1; ++i) {
      for (Int j = i+1; j < m; ++j) {
         const Int result_of_compare = incl(collection[i], collection[j]);
         if (result_of_compare == -1)
            poset.edge(i, j);
         else if (result_of_compare == 1)
            poset.edge(j, i);
      }
   }
   return poset;
}

UserFunction4perl("# @category Posets"
                  "# Enumerate all order preserving maps from one poset to another"
                  "# @param Graph<Directed> P"
                  "# @param Graph<Directed> Q"
                  "# @option Array<Int> prescribed_map A vector of length P.nodes() with those images in Q that should be fixed. Negative entries will be enumerated over."
                  "# @return Array<Array<Int>>",
                  &poset_homomorphisms,
                  "poset_homomorphisms(Graph<Directed>, Graph<Directed> { prescribed_map => [] })");

UserFunction4perl("# @category Posets"
                  "# Count all order preserving maps from one poset to another."
                  "# They are in fact enumerated, but only the count is kept track of using constant memory."
                  "# @param Graph<Directed> P"
                  "# @param Graph<Directed> Q"
                  "# @option Array<Int> prescribed_map A vector of length P.nodes() with those images in Q that should be fixed. Negative entries will be enumerated over."
                  "# @return Int",
                  &n_poset_homomorphisms,
                  "n_poset_homomorphisms(Graph<Directed>, Graph<Directed> { prescribed_map => [] })");
      
UserFunction4perl("# @category Posets"
                  "# Construct the poset of order preserving maps from one poset to another"
                  "# @param Graph<Directed> P"
                  "# @param Graph<Directed> Q"
                  "# @return Graph<Directed>",
                  &hom_poset_pq,
                  "hom_poset(Graph<Directed>, Graph<Directed>)");

UserFunction4perl("# @category Posets"
                  "# Construct the poset of order preserving maps from one poset to another"
                  "# @param Array<Array<Int>> homs"
                  "# @param Graph<Directed> Q"
                  "# @return Graph<Directed>",
                  &hom_poset_hq,
                  "hom_poset(Array<Array<Int>>, Graph<Directed>)");

UserFunction4perl("# @category Posets"
                  "# Construct the covering relations of a poset"
                  "# @param Graph<Directed> P"
                  "# @return Graph<Directed>",
                  &covering_relations,
                  "covering_relations(Graph<Directed>)");

UserFunctionTemplate4perl("# @category Posets"
                          "# Construct the inclusion poset from a given container."
                          "# The elements of the container are interpreted as sets.  They define a poset"
                          "# by inclusion.  The function returns this poset encoded as a directed graph."
                          "# The direction is towards to larger sets.  All relations are encoded, not"
                          "# only the covering relations."
                          "# For details see Assarf, Joswig & Pfeifle:"
                          "# Webs of stars or how to triangulate sums of polytopes, to appear"
                          "# @param Array<T> P"
                          "# @return Graph<Directed>",
                          "poset_by_inclusion<T>(Array<T>)"); 

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
