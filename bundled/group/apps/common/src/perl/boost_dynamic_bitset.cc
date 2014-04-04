/* Copyright (c) 1997-2014
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
#include "polymake/common/boost_dynamic_bitset.h"

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

   Class4perl("Polymake::common::boost_dynamic_bitset", boost_dynamic_bitset);
   FunctionInstance4perl(new, boost_dynamic_bitset);
   OperatorInstance4perl(Binary__eq, perl::Canned< const boost_dynamic_bitset >, perl::Canned< const boost_dynamic_bitset >);
   OperatorInstance4perl(BinaryAssign_sub, perl::Canned< boost_dynamic_bitset >, int);
   OperatorInstance4perl(BinaryAssign_add, perl::Canned< boost_dynamic_bitset >, int);
   FunctionInstance4perl(new_X, boost_dynamic_bitset, int);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
