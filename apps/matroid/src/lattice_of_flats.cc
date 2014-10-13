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
#include "polymake/matroid/lattice_of_flats_tools.h"
#include "polymake/graph/HasseDiagram.h"

namespace polymake { namespace matroid {

//this is a copy from hasse_diagram.cc
template <typename Matrix>
perl::Object lattice_of_flats(const GenericIncidenceMatrix<Matrix>& mat_hyperplanes,const int level=-1)
{
   graph::HasseDiagram LF;
   if(mat_hyperplanes.cols() <= mat_hyperplanes.rows())
      flat_lattice::compute_lattice_of_flats(mat_hyperplanes, filler(LF,true), polytope::face_lattice::Primal(),level);
   else
      flat_lattice::compute_lattice_of_flats(T(mat_hyperplanes), filler(LF,false), polytope::face_lattice::Dual(),level);
   
   return LF.makeObject();
}

FunctionTemplate4perl("lattice_of_flats(IncidenceMatrix; $=-1)");


} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
