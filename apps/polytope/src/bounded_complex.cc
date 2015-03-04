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
#include "polymake/Array.h"
#include "polymake/Set.h"
#include "polymake/polytope/face_lattice_tools.h"
#include "polymake/graph/HasseDiagram.h"
#include "polymake/FacetList.h"
#include "polymake/list"

namespace polymake { namespace polytope {

// TODO: the return value should be a new type PolyhedralComplex

IncidenceMatrix<>
bounded_complex_from_incidence(const IncidenceMatrix<>& VIF, const Set<int>& far_face, const int upper_bound)
{
   graph::HasseDiagram HD;
   face_lattice::compute_bounded(VIF, far_face, filler(HD,true), upper_bound);
   IncidenceMatrix<> BC(HD.max_faces(), VIF.cols());
   BC.squeeze_cols();
   return BC;
}

FacetList
bounded_complex_from_face_lattice(perl::Object HD_obj, const Set<int>& far_face)
{
   const graph::HasseDiagram HD(HD_obj);
   FacetList F(HD.node_range_of_dim(0).size());

   Set<int> faces_to_visit;
   std::list<int> Q;
   copy(entire(HD.node_range_of_dim(-1)), std::back_inserter(Q));

   while (!Q.empty()) {
      const int f=Q.front(); Q.pop_front();
      if ((HD.face(f) * far_face).empty()) {
         F.insertMax(HD.face(f));
      } else {
         for (graph::HasseDiagram::graph_type::in_adjacent_node_list::const_iterator subf=HD.in_adjacent_nodes(f).begin();
              !subf.at_end(); ++subf)
            if (faces_to_visit.collect(*subf))
               Q.push_back(*subf);
      }
   }

   F.squeeze();
   return F;
}

IncidenceMatrix<>
bounded_complex_from_bounded_face_lattice(perl::Object HD_obj)
{
   const graph::HasseDiagram HD(HD_obj);
   IncidenceMatrix<> BC(HD.max_faces());
   BC.squeeze_cols();
   return BC;
}

Function4perl(&bounded_complex_from_incidence, "bounded_complex(IncidenceMatrix Set; $=-1)");
Function4perl(&bounded_complex_from_face_lattice, "bounded_complex(FaceLattice Set)");
Function4perl(&bounded_complex_from_bounded_face_lattice, "bounded_complex(FaceLattice)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
