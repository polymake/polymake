/* Copyright (c) 1997-2018
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

#include "polymake/Graph.h"
#include "polymake/client.h"
#include "polymake/node_edge_incidences.h"

namespace polymake { namespace common { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1>
   FunctionInterface4perl( node_edge_incidences_T_X, T0,T1 ) {
      perl::Value arg0(stack[0]);
      WrapperReturn( (node_edge_incidences<T0>(arg0.get<T1>())) );
   };

   FunctionInstance4perl(node_edge_incidences_T_X, int, perl::Canned< const Graph< Undirected > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
