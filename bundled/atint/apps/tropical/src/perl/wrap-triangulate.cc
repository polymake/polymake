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
   FunctionInterface4perl( insert_rays_T_x_x, T0 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      WrapperReturn( (insert_rays<T0>(arg0, arg1)) );
   };

   template <typename T0>
   FunctionInterface4perl( triangulate_cycle_T_x, T0 ) {
      perl::Value arg0(stack[0]);
      WrapperReturn( (triangulate_cycle<T0>(arg0)) );
   };

   FunctionInstance4perl(triangulate_cycle_T_x, Max);
   FunctionInstance4perl(triangulate_cycle_T_x, Min);
   FunctionInstance4perl(insert_rays_T_x_x, Max);
   FunctionInstance4perl(insert_rays_T_x_x, Min);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
