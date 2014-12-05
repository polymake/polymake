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

#include "polymake/QuadraticExtension.h"
#include "polymake/Rational.h"

namespace polymake { namespace polytope { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1>
   FunctionInterface4perl( pyramid_T_x_C_o, T0,T1 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]);
      WrapperReturn( (pyramid<T0>(arg0, arg1.get<T1, T0>(), arg2)) );
   };

   FunctionInstance4perl(pyramid_T_x_C_o, Rational, int);
   FunctionInstance4perl(pyramid_T_x_C_o, QuadraticExtension< Rational >, int);
   FunctionInstance4perl(pyramid_T_x_C_o, QuadraticExtension< Rational >, perl::Canned< const QuadraticExtension< Rational > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
