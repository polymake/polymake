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

///==== this line controls the automatic file splitting: max.instances=40

#include "polymake/Graph.h"
#include "polymake/IndexedSubgraph.h"
#include "polymake/Set.h"
#include "polymake/client.h"

namespace polymake { namespace common { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1>
   FunctionInterface4perl( induced_subgraph_X8_X8, T0,T1 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      WrapperReturnAnch( 2, (arg0)(arg1), (induced_subgraph(arg0.get<T0>(), arg1.get<T1>())) );
   };

   FunctionInstance4perl(induced_subgraph_X8_X8, perl::Canned< const Wary< Graph< Undirected > > >, perl::Canned< const Set< int > >);
   FunctionInstance4perl(induced_subgraph_X8_X8, perl::Canned< const Wary< Graph< Undirected > > >, perl::Canned< const pm::Series<int, true> >);
   FunctionInstance4perl(induced_subgraph_X8_X8, perl::Canned< const Wary< Graph< Undirected > > >, perl::Canned< const pm::Complement<pm::Set<int, pm::operations::cmp>, int, pm::operations::cmp> >);
   FunctionInstance4perl(induced_subgraph_X8_X8, perl::Canned< const Wary< Graph< Directed > > >, perl::Canned< const pm::Nodes<pm::graph::Graph<pm::graph::Undirected> > >);
   FunctionInstance4perl(induced_subgraph_X8_X8, perl::Canned< const Wary< Graph< Directed > > >, perl::Canned< const pm::Complement<pm::Set<int, pm::operations::cmp>, int, pm::operations::cmp> >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
