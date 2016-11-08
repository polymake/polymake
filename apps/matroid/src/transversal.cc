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
#include "polymake/Bitset.h"
#include "polymake/Array.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Set.h"
#include "polymake/PowerSet.h"
#include "polymake/Graph.h"
#include "polymake/graph/HasseDiagram.h"

namespace polymake { namespace matroid {

   //Returns the set of all node indices such that the corresponding face strictly contains
   //the given set
   // If the face is actually in the lattice, it returns its node index as a second value
   // (which is -1 otherwise).
   std::pair<Set<int>, int> faces_above(const graph::HasseDiagram& G, const Set<int> &face) {
      Set<int> result;
      int face_index = -1;
     
      for(Entire<Nodes<Graph<Directed> > >::const_iterator nodes_it = entire(nodes(G)); 
            !nodes_it.at_end(); nodes_it++) {
        Set<int> gface = G.face( *nodes_it);
        Set<int> inter = gface * face;
        if(inter.size() == face.size()) {
           if(gface.size() > face.size()) 
              result += *nodes_it;
           else
              face_index = *nodes_it;
        }
      }

      return std::make_pair(result, face_index);
   }


   // Computes whether a matroid is transversal and if so, what a presentation is
   // Based on the algorithm in Bonin: An introduction to transversal matroids
   // arXiv: http://home.gwu.edu/~jbonin/TransversalNotes.pdf
   // Note that it is sufficient to check that the multiplicity of a flat is >= 0,
   // instead of checking this for all sets.
   perl::ListReturn check_transversality(perl::Object matroid) {
      const graph::HasseDiagram flats = matroid.give("LATTICE_OF_FLATS");
      const graph::HasseDiagram cyclic_flats = matroid.give("LATTICE_OF_CYCLIC_FLATS");
      int flats_dim = flats.dim();
      int total_rank = matroid.give("RANK");
      int n = matroid.give("N_ELEMENTS");
      Set<int> loops = matroid.give("LOOPS");

      Map<int,int> multiplicity; // Maps cyclic flat indices to multiplicities

      perl::ListReturn r;
      
      for(int i = flats_dim; i >= 0 ; i--) {
         for(Entire<graph::HasseDiagram::nodes_of_dim_set>::const_iterator n_it = entire(flats.node_range_of_dim(i)); !n_it.at_end(); n_it++) {
            std::pair<Set<int>,int> parents = faces_above(cyclic_flats, flats.face(*n_it));
            int mult_value = total_rank - i;
            for(Entire<Set<int> >::const_iterator p = entire(parents.first); !p.at_end(); p++) {
               mult_value -= multiplicity[*p];
            }
            if(mult_value < 0) {
               r << 0; return r;
            }
            if(parents.second != -1) {
               multiplicity[parents.second] = mult_value;
            }
         }
      }
      Vector<Set<int> > potential_presentation;
      Set<int> total_set = sequence(0,n);
      for(Entire<Map<int,int> >::iterator mp = entire(multiplicity); !mp.at_end(); mp++) {
         int this_mult = (*mp).second;
         if(this_mult > 0) {
            Vector<Set<int> > this_face( this_mult);
            this_face.fill( total_set - cyclic_flats.face( (*mp).first));
            potential_presentation |= this_face; 
         }
      }

      r << 1;
      r << potential_presentation;

      return r;
   }

   UserFunction4perl("# @category Advanced properties"
                     "# Checks whether a matroid is transversal."
                     "# If so, returns one possible transversal presentation"
                     "# @param Matroid M"
                     "# @return List(Bool, Array<Set<Int> >)"
                     "# First a bool indicating whether M is transversal"
                     "# If this is true, the second entry is a transversal presentation"
                     "# @example Computes whether the uniform matroid of rank 3 on 4 elements is transversal."
                     "# > @a = check_transversality(uniform_matroid(3,4));"
                     "# > print $a[0];"
                     "# | 1"
                     "# > print $a[1];"
                     "# | {0,1,2,3}"
                     "# | {0,1,2,3}"
                     "# | {0,1,2,3}",
         &check_transversality, "check_transversality(Matroid) : returns(@)");

}}
