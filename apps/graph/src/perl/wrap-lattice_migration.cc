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
#include "polymake/graph/Decoration.h"
#include "polymake/tropical/covectors.h"

namespace polymake { namespace graph { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1>
   FunctionInterface4perl( faces_map_from_decoration_X_X, T0,T1 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      WrapperReturn( (faces_map_from_decoration(arg0.get<T0>(), arg1.get<T1>())) );
   };

   template <typename T0>
   FunctionInterface4perl( migrate_hasse_properties_T_x_f16, T0 ) {
      perl::Value arg0(stack[0]);
      WrapperReturnVoid( (migrate_hasse_properties<T0>(arg0)) );
   };

   FunctionInstance4perl(migrate_hasse_properties_T_x_f16, graph::lattice::Nonsequential);
   FunctionInstance4perl(faces_map_from_decoration_X_X, perl::Canned< const Graph< Directed > >, perl::Canned< const NodeMap< Directed, graph::lattice::BasicDecoration > >);
   FunctionCrossAppInstance4perl(faces_map_from_decoration_X_X, (1, "tropical"), perl::Canned< const Graph< Directed > >, perl::Canned< const NodeMap< Directed, CovectorDecoration > >);
   FunctionInstance4perl(migrate_hasse_properties_T_x_f16, graph::lattice::Sequential);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
