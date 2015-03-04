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


#include "polymake/client.h"
#include "polymake/Rational.h"
#include "polymake/Vector.h"
#include "polymake/SparseVector.h"
#include "polymake/Set.h"
#include "polymake/Map.h"
#include "polymake/Graph.h"
#include "polymake/polytope/sympol_interface.h"
#include "polymake/graph/bipartite.h"

namespace polymake { namespace polytope {


template<typename RefSetType, typename TriangSetType>
SparseVector<int> quotient_of_triangulation(const Array<TriangSetType>& triangulation,
                                            const Array<Array<int> >& gens,
                                            const Array<RefSetType>& reference_array,
                                            perl::OptionSet options)
{
   const bool foldable = options["foldable"];
   const group::PermlibGroup sym_group(gens);
   
   Map<TriangSetType, int> index_of_rep;
   int n_reps(0);
   for (typename Entire<Array<RefSetType> >::const_iterator rit = entire(reference_array); !rit.at_end(); ++rit) 
      index_of_rep[TriangSetType(*rit)] = n_reps++;

   Vector<int> color_of(0);
   if (foldable) {
      perl::Object triangObject("topaz::SimplicialComplex");
      triangObject.take("FACETS") << triangulation;
      const Graph<Undirected> dual_graph = triangObject.give("DUAL_GRAPH.ADJACENCY");
      color_of = graph::bipartite_coloring(dual_graph);
   }

   SparseVector<int> quotient(foldable ? 2*n_reps : n_reps);
   int index_of(0);
   for (typename Entire<Array<TriangSetType> >::const_iterator ait = entire(triangulation); !ait.at_end(); ++ait, ++index_of) {
      const int i = index_of_rep[sym_group.lex_min_representative(*ait)];
      const int j = (!foldable) 
         ? i 
         : (color_of[index_of] 
            ? 2*i 
            : 2*i+1); 
      quotient[j]++;
   }

   return quotient;
}


UserFunctionTemplate4perl("# @category Combinatorics\n"
                          "# In a triangulation T, "
                          "# find the number of representatives of simplices wrt to G,"
                          "# and return the counts in the order indicated by the array R"
                          "# @param Array<Set> T the input triangulation,"
                          "# @param Array<Array<Int>> G the generators of the symmetry group"
                          "# @param Array<Set> R the canonical lex-min representatives of the simplices"
                          "# @option Bool foldable is the triangulation foldable?"
                          "# @return SparseVector V the number of times a simplex G-isomorphic to each representative in R occurs in T",
                          "quotient_of_triangulation<RefSetType,TriangSetType>(Array<TriangSetType> Array<Array<Int>> Array<RefSetType> { foldable => 0 })");

  }}

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:

 
