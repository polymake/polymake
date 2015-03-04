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

#include "polymake/client.h"
#include "polymake/Graph.h"

namespace polymake { namespace common { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0>
   FunctionInterface4perl( contract_edge_x_x_f17, T0 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]);
      WrapperReturnVoid( arg0.get<T0>().contract_edge(arg1, arg2) );
   };

   FunctionInstance4perl(contract_edge_x_x_f17, perl::Canned< Wary< Graph< Undirected > > >);
   FunctionInstance4perl(contract_edge_x_x_f17, perl::Canned< Wary< Graph< Directed > > >);
   FunctionInstance4perl(contract_edge_x_x_f17, perl::Canned< Wary< Graph< UndirectedMulti > > >);
   FunctionInstance4perl(contract_edge_x_x_f17, perl::Canned< Wary< Graph< DirectedMulti > > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
