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

namespace polymake { namespace topaz { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   FunctionWrapper4perl( perl::Object (perl::Object, int, pm::Rational, pm::Rational) ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]), arg3(stack[3]);
      IndirectWrapperReturn( arg0, arg1, arg2.get< perl::TryCanned< const Rational > >(), arg3.get< perl::TryCanned< const Rational > >() );
   }
   FunctionWrapperInstance4perl( perl::Object (perl::Object, int, pm::Rational, pm::Rational) );

   FunctionWrapper4perl( perl::Object (perl::Object, pm::Rational, pm::Rational, int) ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]), arg3(stack[3]);
      IndirectWrapperReturn( arg0, arg1.get< perl::TryCanned< const Rational > >(), arg2.get< perl::TryCanned< const Rational > >(), arg3 );
   }
   FunctionWrapperInstance4perl( perl::Object (perl::Object, pm::Rational, pm::Rational, int) );

   FunctionWrapper4perl( pm::Matrix<pm::Rational> (perl::Object, int) ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      IndirectWrapperReturn( arg0, arg1 );
   }
   FunctionWrapperInstance4perl( pm::Matrix<pm::Rational> (perl::Object, int) );

   FunctionWrapper4perl( perl::Object (perl::Object, int, int) ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]);
      IndirectWrapperReturn( arg0, arg1, arg2 );
   }
   FunctionWrapperInstance4perl( perl::Object (perl::Object, int, int) );

///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
