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
#include "polymake/graph/Decoration.h"

namespace polymake { namespace graph { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0>
   FunctionInterface4perl( new, T0 ) {
      WrapperReturnNew(T0, () );
   };

   template <typename T0, typename T1>
   FunctionInterface4perl( new_X, T0,T1 ) {
      perl::Value arg0(stack[1]);
      WrapperReturnNew(T0, (arg0.get<T1>()) );
   };

   Class4perl("Polymake::graph::InverseRankMap__Nonsequential", graph::lattice::InverseRankMap< graph::lattice::Nonsequential >);
   ClassTemplate4perl("Polymake::graph::InverseRankMap");
   FunctionInstance4perl(new, graph::lattice::InverseRankMap< graph::lattice::Nonsequential >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const graph::lattice::InverseRankMap< graph::lattice::Nonsequential > >, perl::Canned< const graph::lattice::InverseRankMap< graph::lattice::Nonsequential > >);
   FunctionInstance4perl(new_X, graph::lattice::InverseRankMap< graph::lattice::Nonsequential >, perl::Canned< const graph::lattice::InverseRankMap< graph::lattice::Nonsequential > >);
   Class4perl("Polymake::graph::InverseRankMap__Sequential", graph::lattice::InverseRankMap< graph::lattice::Sequential >);
   FunctionInstance4perl(new, graph::lattice::InverseRankMap< graph::lattice::Sequential >);
   OperatorInstance4perl(assign, graph::lattice::InverseRankMap< graph::lattice::Nonsequential >, perl::Canned< const graph::lattice::InverseRankMap< graph::lattice::Sequential > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const graph::lattice::InverseRankMap< graph::lattice::Sequential > >, perl::Canned< const graph::lattice::InverseRankMap< graph::lattice::Sequential > >);
   FunctionInstance4perl(new_X, graph::lattice::InverseRankMap< graph::lattice::Sequential >, perl::Canned< const graph::lattice::InverseRankMap< graph::lattice::Sequential > >);
   OperatorInstance4perl(assign, graph::lattice::InverseRankMap< graph::lattice::Sequential >, perl::Canned< const graph::lattice::InverseRankMap< graph::lattice::Nonsequential > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
