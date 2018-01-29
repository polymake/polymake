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

#include "polymake/Rational.h"
#include "polymake/QuadraticExtension.h"

namespace polymake { namespace polytope { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1, typename T2>
   FunctionInterface4perl( bipyramid_T_x_C_C_o, T0,T1,T2 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]), arg3(stack[3]);
      WrapperReturn( (bipyramid<T0>(arg0, arg1.get<T1, T0>(), arg2.get<T2, T0>(), arg3)) );
   };

   FunctionWrapper4perl( perl::Object (perl::Object, Rational const&, Rational const&, perl::OptionSet) ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]), arg3(stack[3]);
      IndirectWrapperReturn(arg0, arg1.get< perl::TryCanned< const Rational > >(), arg2.get< perl::TryCanned< const Rational > >(), arg3);
   }
   FunctionWrapperInstance4perl( perl::Object (perl::Object, Rational const&, Rational const&, perl::OptionSet) );

   FunctionInstance4perl(bipyramid_T_x_C_C_o, Rational, int, int);
   FunctionInstance4perl(bipyramid_T_x_C_C_o, QuadraticExtension< Rational >, perl::Canned< const QuadraticExtension< Rational > >, perl::Canned< const QuadraticExtension< Rational > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
