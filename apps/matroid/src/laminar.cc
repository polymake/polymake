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
#include "polymake/IncidenceMatrix.h"
#include "polymake/Set.h"
#include "polymake/graph/Lattice.h"
#include "polymake/graph/Decoration.h"

namespace polymake { namespace matroid {

   using graph::Lattice;
   using graph::lattice::Sequential;
   using graph::lattice::BasicDecoration;

   // Assumes that c1.size() <= c2.size()
   bool check_circuit_compatibility(const Set<int> &c1, const Set<int> &c2, const Lattice<BasicDecoration, Sequential>& hd) {
      const auto relevant_faces = hd.nodes_of_rank( c2.size() - 1);
      Set<int> closure;
      for(const auto& rf : relevant_faces) {
         const auto& rf_face = hd.face( rf );
         //Check if the flat contains c2
         if( incl(c2, rf_face) <= 0 ) {
            if( closure.empty() ) closure = rf_face;
            else closure *= rf_face;
         }
      }
      return incl(c1, closure) <= 0; //Check that the closure of c2 contains c1.
   }

   //Checks whether a matroid is laminar
   bool is_laminar_matroid(perl::Object matroid) {
      perl::Object lattice_of_flats_obj = matroid.give("LATTICE_OF_FLATS");
      Lattice<BasicDecoration, Sequential> lattice_of_flats(lattice_of_flats_obj);
      IncidenceMatrix<> circuits = matroid.give("CIRCUITS");

      for(auto c1 = entire(rows(circuits)); !c1.at_end(); ++c1) {
         for(auto c2 = c1; !(++c2).at_end();) {
            if( !((*c1) * (*c2)).empty()) {
               const bool flip = (*c2).size() < (*c1).size();
               if(!check_circuit_compatibility( flip? (*c2) : (*c1), flip? (*c1) : (*c2),
                        lattice_of_flats )) return false;
            }
         }
      }
      return true;
   }

   Function4perl(&is_laminar_matroid, "is_laminar_matroid(Matroid)");

}}
