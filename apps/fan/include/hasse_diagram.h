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

#ifndef POLYMAKE_FAN_HASSE_DIAGRAM_H
#define POLYMAKE_FAN_HASSE_DIAGRAM_H

#include "polymake/hash_map"
#include "polymake/FaceMap.h"
#include "polymake/Bitset.h"
#include "polymake/FacetList.h"
#include "polymake/graph/HasseDiagram.h"
namespace polymake { namespace fan {

   graph::HasseDiagram hasse_diagram_fan_computation(const IncidenceMatrix<> &MaximalCones, 
                           const Array<IncidenceMatrix<> > &ListOfCones, 
                           const Array<int> dims, 
                           const int dim,
                           const int to_dim);
}}

#endif
