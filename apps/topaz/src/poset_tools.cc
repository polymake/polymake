/* Copyright (c) 1997-2019
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
#include "polymake/topaz/poset_tools.h"

namespace polymake { namespace topaz {

Array<Array<int>>
poset_homomorphisms(const perl::Object p, const perl::Object q, perl::OptionSet options)
{
   const Graph<Directed> 
      P = p.give("ADJACENCY"),
      Q = q.give("ADJACENCY");
   const Array<int> prescribed_map = options["prescribed_map"];

   RecordKeeper<HomList> record_keeper;

   const HomList result(poset_homomorphisms_impl(P, Q, record_keeper, prescribed_map));
   return Array<Array<int>>(result.size(), entire(result));
}

int
n_poset_homomorphisms(const perl::Object p, const perl::Object q, perl::OptionSet options)
{
   const Graph<Directed> 
      P = p.give("ADJACENCY"),
      Q = q.give("ADJACENCY");
   const Array<int> prescribed_map = options["prescribed_map"];

   RecordKeeper<int> record_keeper;

   return poset_homomorphisms_impl(P, Q, record_keeper, prescribed_map);
}
      
Graph<Directed> hom_poset_pq(const perl::Object p, const perl::Object q)
{
   const Graph<Directed> 
      P = p.give("ADJACENCY"),
      Q = q.give("ADJACENCY");
   return hom_poset_impl(P, Q);
}

Graph<Directed> hom_poset_hq(const Array<Array<int>>& homs, const perl::Object q)
{
   const Graph<Directed> Q = q.give("ADJACENCY");
   return hom_poset_impl(homs, Q);
}

Graph<Directed> covering_relations(const perl::Object p)
{
   const Graph<Directed> P = p.give("ADJACENCY");
   return covering_relations_impl(P);
}


UserFunction4perl("# @category Producing a new simplicial complex from others\n"
                  "# Enumerate all order preserving maps from one poset to another"
                  "# @param Graph<Directed> P"
                  "# @param Graph<Directed> Q"
                  "# @option Array<Int> prescribed_map A vector of length P.nodes() with those images in Q that should be fixed. Negative entries will be enumerated over."
                  "# @return Array<Array<Int>>",
                  &poset_homomorphisms,
                  "poset_homomorphisms(Graph<Directed>, Graph<Directed> { prescribed_map => []  })"); 

UserFunction4perl("# @category Combinatorics\n"
                  "# Count all order preserving maps from one poset to another."
                  "# They are in fact enumerated, but only the count is kept track of using constant memory."
                  "# @param Graph<Directed> P"
                  "# @param Graph<Directed> Q"
                  "# @option Array<Int> prescribed_map A vector of length P.nodes() with those images in Q that should be fixed. Negative entries will be enumerated over."
                  "# @return Int",
                  &n_poset_homomorphisms,
                  "n_poset_homomorphisms(Graph<Directed>, Graph<Directed> { prescribed_map => []  })"); 
      
UserFunction4perl("# @category Producing a new simplicial complex from others\n"
                  "# Construct the poset of order preserving maps from one poset to another"
                  "# @param Graph<Directed> P"
                  "# @param Graph<Directed> Q"
                  "# @return Graph<Directed>",
                  &hom_poset_pq,
                  "hom_poset(Graph<Directed>, Graph<Directed>)"); 

UserFunction4perl("# @category Producing a new simplicial complex from others\n"
                  "# Construct the poset of order preserving maps from one poset to another"
                  "# @param Array<Array<Int>> homs"
                  "# @param Graph<Directed> Q"
                  "# @return Graph<Directed>",
                  &hom_poset_hq,
                  "hom_poset(Array<Array<Int>>, Graph<Directed>)"); 

UserFunction4perl("# @category Producing a new simplicial complex from others\n"
                  "# Construct the covering relations of a poset"
                  "# @param Graph<Directed> P"
                  "# @return Graph<Directed>",
                  &covering_relations,
                  "covering_relations(Graph<Directed>)"); 

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
