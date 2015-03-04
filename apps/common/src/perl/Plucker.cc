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
#include "polymake/Plucker.h"
#include "polymake/Rational.h"
#include "polymake/Vector.h"
namespace polymake { namespace common { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1>
   FunctionInterface4perl( new_X, T0,T1 ) {
      perl::Value arg0(stack[1]);
      WrapperReturnNew(T0, (arg0.get<T1>()) );
   };

   template <typename T0, typename T1>
   FunctionInterface4perl( new_int_int_X, T0,T1 ) {
      perl::Value arg0(stack[1]), arg1(stack[2]), arg2(stack[3]);
      WrapperReturnNew(T0, (arg0.get<int>(), arg1.get<int>(), arg2.get<T1>()) );
   };

   ClassTemplate4perl("Polymake::common::Plucker");
   Class4perl("Polymake::common::Plucker__Rational", Plucker< Rational >);
   FunctionInstance4perl(new_X, Plucker< Rational >, perl::Canned< const Vector< Rational > >);
   FunctionInstance4perl(new_int_int_X, Plucker< Rational >, perl::Canned< const Vector< Rational > >);
   OperatorInstance4perl(Binary_add, perl::Canned< const Plucker< Rational > >, perl::Canned< const Plucker< Rational > >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Plucker< Rational > >, perl::Canned< const Plucker< Rational > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
