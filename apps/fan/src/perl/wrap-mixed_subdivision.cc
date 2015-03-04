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

#include "polymake/Rational.h"
#include "polymake/Array.h"
#include "polymake/Set.h"
#include "polymake/linalg.h"

namespace polymake { namespace fan { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1, typename T2>
   FunctionInterface4perl( mixed_subdivision_T_x_X_X_o, T0,T1,T2 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]), arg3(stack[3]);
      WrapperReturn( (mixed_subdivision<T0>(arg0, arg1.get<T1>(), arg2.get<T2>(), arg3)) );
   };

   template <typename T0, typename T1, typename T2>
   FunctionInterface4perl( mixed_subdivision_T_x_x_X_X, T0,T1,T2 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]), arg3(stack[3]);
      WrapperReturn( (mixed_subdivision<T0>(arg0, arg1, arg2.get<T1>(), arg3.get<T2>())) );
   };

   FunctionInstance4perl(mixed_subdivision_T_x_x_X_X, Rational, perl::Canned< const Array< Set< int > > >, perl::Canned< const pm::SameElementVector<pm::Rational const&> >);
   FunctionInstance4perl(mixed_subdivision_T_x_X_X_o, Rational, perl::Canned< const Array< Set< int > > >, perl::Canned< const pm::SameElementVector<pm::Rational const&> >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
