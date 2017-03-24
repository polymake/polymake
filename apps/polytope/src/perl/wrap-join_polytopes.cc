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

namespace polymake { namespace polytope { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0>
   FunctionInterface4perl( free_sum_impl_T_x_x_x_x_x_o, T0 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]), arg3(stack[3]), arg4(stack[4]), arg5(stack[5]);
      WrapperReturn( (free_sum_impl<T0>(arg0, arg1, arg2, arg3, arg4, arg5)) );
   };

   template <typename T0>
   FunctionInterface4perl( join_polytopes_T_x_x_o, T0 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]);
      WrapperReturn( (join_polytopes<T0>(arg0, arg1, arg2)) );
   };

   FunctionInstance4perl(join_polytopes_T_x_x_o, Rational);
   FunctionInstance4perl(free_sum_impl_T_x_x_x_x_x_o, Rational);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
