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

#include "polymake/Array.h"
#include "polymake/Set.h"
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

   FunctionInstance4perl(new_X, Array< Set< int > >, perl::Canned< const Set< Set< int > > >);
   Class4perl("Polymake::common::Array__Array__boost_dynamic_bitset", Array< Array< boost_dynamic_bitset > >);
   Class4perl("Polymake::common::Array__boost_dynamic_bitset", Array< boost_dynamic_bitset >);
   FunctionInstance4perl(new_X, Array< Array< Set< int > > >, perl::Canned< const Array< Array< boost_dynamic_bitset > > >);
   FunctionInstance4perl(new, Array< boost_dynamic_bitset >);
   FunctionInstance4perl(new_X, Array< boost_dynamic_bitset >, perl::Canned< const Array< boost_dynamic_bitset > >);
   FunctionInstance4perl(new, Array< Array< boost_dynamic_bitset > >);
   FunctionInstance4perl(new_X, Array< boost_dynamic_bitset >, int);
   FunctionInstance4perl(new_X, Array< Set< int > >, perl::Canned< const Array< boost_dynamic_bitset > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Array< boost_dynamic_bitset > >, perl::Canned< const Array< boost_dynamic_bitset > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Array< Array< boost_dynamic_bitset > > >, perl::Canned< const Array< Array< boost_dynamic_bitset > > >);
   OperatorInstance4perl(convert, Array< boost_dynamic_bitset >, perl::Canned< const Array< Set< int > > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
