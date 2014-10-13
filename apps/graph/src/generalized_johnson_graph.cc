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

// the family of Graphs J(n,k,i) from [Godsil & Royle - algebraic graph theory]
// with nodes [n] choose k and an edge between two nodes if there intersection has size i.  
// i=0 gives the Kneser Graph and i=k-1 the Johnson Graph.
perl::Object generalized_johnson_graph (const int n,const int k,const int i) {

   if (n < 1)
      throw std::runtime_error("generalized_johnson_graph: n should be positiv");
   if (k < 1 || k > n)
      throw std::runtime_error("generalized_johnson_graph: 1 <= k <= n required");


   perl::Object JGraph("Graph<Undirected>");
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
      
   Graph<> jgraph(ct);
      
   for(Entire<Map<Set<int>, int> >::const_iterator mit1 = entire(index_of); !mit1.at_end(); ++mit1) 
      for(Entire<Map<Set<int>, int> >::const_iterator mit2 = mit1; !mit2.at_end(); ++mit2) 
         if ((mit1->first * mit2->first).size()==i)
	    jgraph.edge(mit1->second, mit2->second);

   JGraph.take("ADJACENCY") << jgraph;
   JGraph.take("N_NODES") << ct;
   JGraph.take("N_EDGES") << jgraph.edges();
   JGraph.take("NODE_LABELS") << labels;
   return JGraph;
}

perl::Object kneser_graph(const int n, const int k){
   return generalized_johnson_graph(n,k,0);
}

perl::Object johnson_graph(const int n, const int k){
   return generalized_johnson_graph(n,k,k-1);
}

UserFunction4perl("# @category Producing from scratch"
                  "# Create the __generalized Johnson graph__ on parameters (n,k,i)."
                  "#   It has one node for each set in \\({[n]}\\choose{k}\\),"
                  "#   and an edge between two nodes iff the intersection of the corresponding subsets is of size i."
                  "# @param Int n the size of the ground set"
                  "# @param Int k the size of the subsets"
                  "# @param Int i the size of the subsets"
                  "# @return Graph",
                  &generalized_johnson_graph, "generalized_johnson_graph($$$)");

UserFunction4perl("# @category Producing from scratch"
                  "# Create the __Kneser graph__ on parameters (n,k)."
                  "#   It has one node for each set in \\({[n]}\\choose{k}\\),"
                  "#   and an edge between two nodes iff the corresponding subsets are disjoint."
                  "# @param Int n the size of the ground set"
                  "# @param Int k the size of the subsets"
                  "# @return Graph",
                  &kneser_graph, "kneser_graph($$)");


UserFunction4perl("# @category Producing from scratch"
                  "# Create the __Johnson graph__ on parameters (n,k)."
                  "#   It has one node for each set in \\({[n]}\\choose{k}\\),"
                  "#   and an edge between two nodes iff the intersection of the corresponding subsets is of size k-1."
                  "# @param Int n the size of the ground set"
                  "# @param Int k the size of the subsets"
                  "# @return Graph",
                  &johnson_graph, "johnson_graph($$)");

} }


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
