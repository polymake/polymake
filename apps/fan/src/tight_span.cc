/* Copyright (c) 1997-2016
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
#include "polymake/fan/tight_span.h"

namespace polymake { namespace fan {


   FacetList tight_span_from_face_lattice(perl::Object HD_obj, Array< Set<int> > boundary)
   {
      const graph::HasseDiagram HD(HD_obj);
      FacetList F(HD.node_range_of_dim(0).size());

      Set<int> current_faces;
      Set<int> next_faces;
      const Set< Set<int> > bd( boundary );
      std::list<int> L;
      for (auto it=entire(HD.node_range_of_dim( HD.dim()-2 )); !it.at_end(); ++it){
         if(HD.out_adjacent_nodes(*it).size() > 1  || bd.contains(HD.face(*it)) ){
            L.push_back(*it);
            current_faces+=*it;
         }
      }

      if(L.empty()){
         F.insertMax(HD.dual_faces()[HD.node_range_of_dim( HD.dim()-1 ).front()]);
      }

      int rank(HD.dim()-2);
      while (!L.empty()) {
         const int f=L.front(); L.pop_front();
         if( HD.dim_of_node(f)<rank ){
            current_faces=next_faces;
            next_faces.clear();
            if(rank==1)
               current_faces.clear();
            else
               --rank;
         }
         bool minimal = true;
         for (auto subf=HD.in_adjacent_nodes(f).begin(); !subf.at_end(); ++subf)
            if (incl( HD.out_adjacent_nodes(*subf), current_faces) < 1 ){
               minimal = false;
               if(!next_faces.collect(*subf))
                  L.push_back(*subf);
            }
         if(minimal){
            Set<int> temp;
            for(int i=0; i<boundary.size(); ++i)
               if( incl(HD.face(f),boundary[i]) < 1 )
                  temp+=(i+HD.node_range_of_dim(0).size());
            F.insertMax(HD.dual_faces()[f]+temp);
         }
      }

      F.squeeze();
      return F;
   }
   
   FunctionTemplate4perl("tight_span_from_incidence_with_excluded_faces(IncidenceMatrix, Set<Set<Int>>; $=-1)");
   FunctionTemplate4perl("tight_span_from_incidence(IncidenceMatrix, Array<IncidenceMatrix>, Array<Int>, $; $=-1)");

   FunctionTemplate4perl("tight_span_vertices<Scalar>(Matrix<Scalar>, IncidenceMatrix, Vector<Scalar>)");
   Function4perl(&tight_span_from_face_lattice, "tight_span(FaceLattice; Array<Set>=[] )");

   InsertEmbeddedRule("function tight_span(IncidenceMatrix, Set<Set<Int>>;$=-1) {\n"
         "  return tight_span_from_incidence_with_excluded_faces(@_);}\n");

   InsertEmbeddedRule("function tight_span(IncidenceMatrix, Array<IncidenceMatrix>, Array<Int>,$; $=-1) {\n"
         "  return tight_span_from_incidence(@_);}\n");

}
}
