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
#include "polymake/polytope/face_lattice_tools.h"
#include "polymake/graph/HasseDiagram.h"

namespace polymake { namespace polytope {

template <typename Matrix>
perl::Object hasse_diagram(const GenericIncidenceMatrix<Matrix>& VIF, int dim_upper_bound=-1)
{
   graph::HasseDiagram HD;
   if (dim_upper_bound>=0 || VIF.cols() <= VIF.rows())
      face_lattice::compute(VIF, filler(HD), face_lattice::Primal(), dim_upper_bound);
   else
      face_lattice::compute(T(VIF), filler(HD), face_lattice::Dual());
   return HD.makeObject();
}

template <typename Matrix, typename Set>
perl::Object bounded_hasse_diagram(const GenericIncidenceMatrix<Matrix>& VIF, const GenericSet<Set>& far_face, int dim_upper_bound=-1)
{
   graph::HasseDiagram HD;
   face_lattice::compute_bounded(VIF, far_face, filler(HD), dim_upper_bound);
   return HD.makeObject();
}

FunctionTemplate4perl("hasse_diagram(IncidenceMatrix; $=-1)");
FunctionTemplate4perl("bounded_hasse_diagram(IncidenceMatrix Set; $=-1)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
