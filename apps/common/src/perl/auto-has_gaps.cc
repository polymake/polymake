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
   template <typename T0>
   FunctionInterface4perl( has_gaps_f1, T0 ) {
      perl::Value arg0(stack[0]);
      WrapperReturn( arg0.get<T0>().has_gaps() );
   };

   FunctionInstance4perl(has_gaps_f1, perl::Canned< const Graph< Undirected > >);
   FunctionInstance4perl(has_gaps_f1, perl::Canned< const Graph< Directed > >);
   FunctionInstance4perl(has_gaps_f1, perl::Canned< const Graph< DirectedMulti > >);
   FunctionInstance4perl(has_gaps_f1, perl::Canned< const Graph< UndirectedMulti > >);
   FunctionInstance4perl(has_gaps_f1, perl::Canned< const pm::IndexedSubgraph<pm::graph::Graph<pm::graph::Undirected> const&, pm::Set<int, pm::operations::cmp> const&, mlist<> > >);
   FunctionInstance4perl(has_gaps_f1, perl::Canned< const pm::IndexedSubgraph<pm::graph::Graph<pm::graph::Undirected> const&, pm::Complement<pm::Set<int, pm::operations::cmp>, int, pm::operations::cmp> const&, mlist<> > >);
   FunctionInstance4perl(has_gaps_f1, perl::Canned< const pm::IndexedSubgraph<pm::graph::Graph<pm::graph::Undirected> const&, pm::Series<int, true> const&, mlist<pm::RenumberTag<std::integral_constant<bool, true> > > > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
