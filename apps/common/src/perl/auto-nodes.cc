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

#include "polymake/Graph.h"
#include "polymake/IndexedSubgraph.h"
#include "polymake/Set.h"
#include "polymake/client.h"

namespace polymake { namespace common { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0>
   FunctionInterface4perl( nodes_R_X32, T0 ) {
      perl::Value arg0(stack[1]);
      WrapperReturnPkg( (nodes(arg0.get<T0>())), arg0 );
   };

   template <typename T0>
   FunctionInterface4perl( nodes_f1, T0 ) {
      perl::Value arg0(stack[0]);
      WrapperReturn( arg0.get<T0>().nodes() );
   };

   FunctionInstance4perl(nodes_f1, perl::Canned< const Graph< Undirected > >);
   FunctionInstance4perl(nodes_f1, perl::Canned< const Graph< Directed > >);
   FunctionInstance4perl(nodes_f1, perl::Canned< const Graph< DirectedMulti > >);
   FunctionInstance4perl(nodes_R_X32, perl::Canned< const Graph< Undirected > >);
   FunctionInstance4perl(nodes_R_X32, perl::Canned< const Graph< Directed > >);
   FunctionInstance4perl(nodes_R_X32, perl::Canned< const Graph< DirectedMulti > >);
   FunctionInstance4perl(nodes_R_X32, perl::Canned< const Graph< UndirectedMulti > >);
   FunctionInstance4perl(nodes_R_X32, perl::Canned< const pm::IndexedSubgraph<pm::graph::Graph<pm::graph::Undirected> const&, pm::Set<int, pm::operations::cmp> const&, mlist<> > >);
   FunctionInstance4perl(nodes_R_X32, perl::Canned< const pm::IndexedSubgraph<pm::graph::Graph<pm::graph::Undirected> const&, pm::Complement<pm::Set<int, pm::operations::cmp>, int, pm::operations::cmp> const&, mlist<> > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
