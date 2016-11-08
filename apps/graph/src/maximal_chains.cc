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
#include "polymake/Set.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Graph.h"

namespace polymake { namespace graph {

   IncidenceMatrix<> maximal_chains(perl::Object face_lattice) {

      int total_dim = face_lattice.call_method("dim");
      Graph<Directed> face_graph = face_lattice.give("ADJACENCY");

      //Prepare variables for chains, start with one chain with element indexof(emptyset)
      //(that will correspond to the vertex later)
      //Each chain is an ordered list of indices, referring to FACES
      //Top element saves the index of the top element of each chain.

      int bottom_index = face_lattice.call_method("bottom_node"); 
      IncidenceMatrix<> chains_as_sets(0, bottom_index+1);
      std::list<int> top_element;
      chains_as_sets /= scalar2set(bottom_index);
      top_element.push_back(bottom_index);

      for(int d = 1; d <= total_dim; ++d) {
         RestrictedIncidenceMatrix<> new_chains_as_sets; 
         std::list<int> new_top_element;
         //For each chain, find each possible continuation.
         auto chain = entire(rows(chains_as_sets));
         for(auto chain_top = entire(top_element); !chain_top.at_end(); ++chain_top, ++chain) {
            //Find adjacent elements of each top element
            Set<int> neighbours = face_graph.out_adjacent_nodes(*chain_top);
            for(auto nb : neighbours) {
               new_chains_as_sets /= (  (*chain) + nb);
               new_top_element.push_back( nb);
            }
         }
         chains_as_sets = std::move(new_chains_as_sets);
         top_element = new_top_element;
      }

      return chains_as_sets;

   }

   UserFunction4perl("# @category Combinatorics"
                     "# Computes the set of maximal chains of a FaceLattice object."
                     "# Note that all such chains begin at the bottom node and end in nodes of"
                     "# dimension F->dim()"
                     "# @param FaceLattice F"
                     "# @return IncidenceMatrix Each row is a maximal chain, "
                     "# indices refer to nodes of the FaceLattice",
                     &maximal_chains, "maximal_chains(FaceLattice)");

}}
