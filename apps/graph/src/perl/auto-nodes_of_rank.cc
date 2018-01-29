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
   FunctionInterface4perl( nodes_of_rank_x_f1, T0 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      WrapperReturn( arg0.get<T0>().nodes_of_rank(arg1) );
   };

   FunctionInstance4perl(nodes_of_rank_x_f1, perl::Canned< const graph::lattice::InverseRankMap< graph::lattice::Nonsequential > >);
   FunctionInstance4perl(nodes_of_rank_x_f1, perl::Canned< const graph::lattice::InverseRankMap< graph::lattice::Sequential > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
