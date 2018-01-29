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
#include "polymake/Graph.h"

namespace polymake { namespace common { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1>
   FunctionInterface4perl( new_X, T0,T1 ) {
      perl::Value arg0(stack[1]);
      WrapperReturnNew(T0, (arg0.get<T1>()) );
   };

   ClassTemplate4perl("Polymake::common::NodeHashMap");
   Class4perl("Polymake::common::NodeHashMap_A_Undirected_I_Bool_Z", NodeHashMap< Undirected, bool >);
   FunctionInstance4perl(new_X, NodeHashMap< Undirected, bool >, perl::Canned< const Graph< Undirected > >);
   OperatorInstance4perl(Binary_brk, perl::Canned< NodeHashMap< Undirected, bool > >, int);
   Class4perl("Polymake::common::NodeHashMap_A_Directed_I_Bool_Z", NodeHashMap< Directed, bool >);
   FunctionInstance4perl(new_X, NodeHashMap< Directed, bool >, perl::Canned< const Graph< Directed > >);
   OperatorInstance4perl(Binary_brk, perl::Canned< NodeHashMap< Directed, bool > >, int);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
