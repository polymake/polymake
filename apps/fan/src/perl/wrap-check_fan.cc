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

namespace polymake { namespace fan { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0>
   FunctionInterface4perl( check_fan_objects_T_x_o, T0 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      WrapperReturn( (check_fan_objects<T0>(arg0, arg1)) );
   };

   FunctionWrapper4perl( perl::Object (pm::Matrix<pm::Rational> const&, pm::Array<pm::Set<int, pm::operations::cmp>> const&, perl::OptionSet) ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]);
      IndirectWrapperReturn( arg0.get< perl::TryCanned< const Matrix< Rational > > >(), arg1.get< perl::TryCanned< const Array< Set< int > > > >(), arg2 );
   }
   FunctionWrapperInstance4perl( perl::Object (pm::Matrix<pm::Rational> const&, pm::Array<pm::Set<int, pm::operations::cmp>> const&, perl::OptionSet) );

   FunctionInstance4perl(check_fan_objects_T_x_o, Rational);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
