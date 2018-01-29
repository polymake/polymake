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

#include "polymake/TropicalNumber.h"

namespace polymake { namespace tropical { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0>
   FunctionInterface4perl( local_m0n_T_x, T0 ) {
      perl::Value arg0(stack[0]);
      WrapperReturn( (local_m0n<T0>(arg0)) );
   };

   template <typename T0, typename T1>
   FunctionInterface4perl( local_m0n_wrap_T_C_x, T0,T1 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      WrapperReturn( (local_m0n_wrap<T0>(arg0.get<T1, T0>(), arg1)) );
   };

   FunctionInstance4perl(local_m0n_wrap_T_C_x, Max, perl::Canned< const Max >);
   FunctionInstance4perl(local_m0n_wrap_T_C_x, Min, perl::Canned< const Min >);
   FunctionInstance4perl(local_m0n_T_x, Max);
   FunctionInstance4perl(local_m0n_T_x, Min);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
