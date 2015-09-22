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

#include "polymake/TropicalNumber.h"

namespace polymake { namespace tropical { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0>
   FunctionInterface4perl( space_of_stable_maps_T_x_x_x, T0 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]);
      WrapperReturn( (space_of_stable_maps<T0>(arg0, arg1, arg2)) );
   };

   template <typename T0>
   FunctionInterface4perl( m0n_T_x, T0 ) {
      perl::Value arg0(stack[0]);
      WrapperReturn( (m0n<T0>(arg0)) );
   };

   template <typename T0, typename T1>
   FunctionInterface4perl( m0n_wrap_T_x_C, T0,T1 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      WrapperReturn( (m0n_wrap<T0>(arg0, arg1.get<T1, T0>())) );
   };

   FunctionWrapper4perl( pm::Integer (int, int) ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      IndirectWrapperReturn( arg0, arg1 );
   }
   FunctionWrapperInstance4perl( pm::Integer (int, int) );

   FunctionInstance4perl(m0n_wrap_T_x_C, Max, perl::Canned< const Max >);
   FunctionInstance4perl(m0n_wrap_T_x_C, Min, perl::Canned< const Min >);
   FunctionInstance4perl(m0n_T_x, Min);
   FunctionInstance4perl(m0n_T_x, Max);
   FunctionInstance4perl(space_of_stable_maps_T_x_x_x, Max);
   FunctionInstance4perl(space_of_stable_maps_T_x_x_x, Min);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
