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

///==== this line controls the automatic file splitting: max.instances=40

#include "polymake/client.h"
#include "polymake/graph/compare.h"
#include "polymake/Graph.h"

namespace polymake { namespace graph { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1>
   FunctionInterface4perl( find_node_permutation_X_X, T0,T1 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      WrapperReturn( (find_node_permutation(arg0.get<T0>(), arg1.get<T1>())) );
   };

   FunctionInstance4perl(find_node_permutation_X_X, perl::Canned< const Graph< Undirected > >, perl::Canned< const Graph< Undirected > >);
   FunctionInstance4perl(find_node_permutation_X_X, perl::Canned< const Graph< Directed > >, perl::Canned< const Graph< Directed > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
