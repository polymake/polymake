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

#include "polymake/Bitset.h"
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

   Class4perl("Polymake::common::Bitset", Bitset);
   FunctionInstance4perl(new, Bitset);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Bitset >, perl::Canned< const Bitset >);
   OperatorInstance4perl(BinaryAssign_add, perl::Canned< Bitset >, int);
   OperatorInstance4perl(BinaryAssign_sub, perl::Canned< Bitset >, int);
   FunctionInstance4perl(new_X, Bitset, perl::Canned< const Bitset >);
   FunctionInstance4perl(new_X, Bitset, perl::Canned< const Set< int > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
