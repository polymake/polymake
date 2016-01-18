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
#include "polymake/IncidenceMatrix.h"
#include "polymake/IndexedSubgraph.h"
#include "polymake/Matrix.h"
#include "polymake/Rational.h"
#include "polymake/Set.h"
#include "polymake/client.h"

namespace polymake { namespace common { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1>
   FunctionInterface4perl( new_X, T0,T1 ) {
      perl::Value arg0(stack[1]);
      WrapperReturnNew(T0, (arg0.get<T1>()) );
   };

   template <typename T0>
   FunctionInterface4perl( new, T0 ) {
      WrapperReturnNew(T0, () );
   };

   template <typename T0>
   FunctionInterface4perl( new_int, T0 ) {
      perl::Value arg0(stack[1]);
      WrapperReturnNew(T0, (arg0.get<int>()) );
   };

   ClassTemplate4perl("Polymake::common::Graph");
   Class4perl("Polymake::common::Graph__Undirected", Graph< Undirected >);
   Class4perl("Polymake::common::Graph__Directed", Graph< Directed >);
   FunctionInstance4perl(new, Graph< Undirected >);
   FunctionInstance4perl(new, Graph< Directed >);
   FunctionInstance4perl(new_int, Graph< Undirected >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Graph< Undirected > >, perl::Canned< const Graph< Undirected > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Graph< Undirected > >, perl::Canned< const pm::IndexedSubgraph<pm::graph::Graph<pm::graph::Undirected> const&, pm::Set<int, pm::operations::cmp> const&, void> >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Graph< Undirected > >, perl::Canned< const pm::IndexedSubgraph<pm::graph::Graph<pm::graph::Undirected> const&, pm::Series<int, true> const&, pm::Renumber<pm::bool2type<true> > > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Graph< Undirected > >, perl::Canned< const pm::IndexedSubgraph<pm::graph::Graph<pm::graph::Undirected> const&, pm::Complement<pm::Set<int, pm::operations::cmp>, int, pm::operations::cmp> const&, void> >);
   FunctionInstance4perl(new_X, Graph< Undirected >, perl::Canned< const Graph< Undirected > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Graph< Directed > >, perl::Canned< const Graph< Directed > >);
   OperatorInstance4perl(convert, Graph< Directed >, perl::Canned< const Graph< Undirected > >);
   FunctionInstance4perl(new_X, Graph< Undirected >, perl::Canned< const pm::IndexedSubgraph<pm::graph::Graph<pm::graph::Undirected> const&, pm::Complement<pm::Set<int, pm::operations::cmp>, int, pm::operations::cmp> const&, void> >);
   FunctionInstance4perl(new_X, Graph< Directed >, perl::Canned< const pm::IndexedSubgraph<pm::graph::Graph<pm::graph::Directed> const&, pm::Complement<pm::Set<int, pm::operations::cmp>, int, pm::operations::cmp> const&, void> >);
   FunctionInstance4perl(new_X, Graph< Undirected >, perl::Canned< const pm::IndexedSubgraph<pm::graph::Graph<pm::graph::Undirected> const&, pm::Series<int, true> const&, pm::Renumber<pm::bool2type<true> > > >);
   FunctionInstance4perl(new_X, Graph< Directed >, perl::Canned< const Graph< Directed > >);
   OperatorInstance4perl(Binary_sub, perl::Canned< const Wary< Graph< Undirected > > >, perl::Canned< const Graph< Directed > >);
   OperatorInstance4perl(Binary_sub, perl::Canned< const Wary< Graph< Undirected > > >, perl::Canned< const Graph< Undirected > >);
   FunctionInstance4perl(new_int, Graph< Directed >);
   FunctionInstance4perl(new_X, Graph< Undirected >, perl::Canned< const Graph< Directed > >);
   Class4perl("Polymake::common::Graph__DirectedMulti", Graph< DirectedMulti >);
   FunctionInstance4perl(new, Graph< DirectedMulti >);
   FunctionInstance4perl(new_X, Graph< DirectedMulti >, perl::Canned< const Graph< DirectedMulti > >);
   Class4perl("Polymake::common::Graph__UndirectedMulti", Graph< UndirectedMulti >);
   FunctionInstance4perl(new, Graph< UndirectedMulti >);
   FunctionInstance4perl(new_X, Graph< Undirected >, perl::Canned< const pm::IndexedSubgraph<pm::graph::Graph<pm::graph::Directed> const&, pm::Nodes<pm::graph::Graph<pm::graph::Undirected> > const&, void> >);
   FunctionInstance4perl(new_X, Graph< Undirected >, perl::Canned< const IncidenceMatrix< Symmetric > >);
   OperatorInstance4perl(convert, Graph< Undirected >, perl::Canned< const Graph< Directed > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
