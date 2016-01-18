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

#include "polymake/IncidenceMatrix.h"
#include "polymake/Integer.h"
#include "polymake/Matrix.h"
#include "polymake/Rational.h"
#include "polymake/TropicalNumber.h"
#include "polymake/Vector.h"

namespace polymake { namespace tropical { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0>
   FunctionInterface4perl( cross_variety_T_x_x_x_x, T0 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]), arg3(stack[3]);
      WrapperReturn( (cross_variety<T0>(arg0, arg1, arg2, arg3)) );
   };

   template <typename T0>
   FunctionInterface4perl( affine_linear_space_T_x_x_x, T0 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]);
      WrapperReturn( (affine_linear_space<T0>(arg0, arg1, arg2)) );
   };

   template <typename T0>
   FunctionInterface4perl( orthant_subdivision_T_x_x_x, T0 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]);
      WrapperReturn( (orthant_subdivision<T0>(arg0, arg1, arg2)) );
   };

   template <typename T0>
   FunctionInterface4perl( projective_torus_T_x_x, T0 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      WrapperReturn( (projective_torus<T0>(arg0, arg1)) );
   };

   template <typename T0>
   FunctionInterface4perl( halfspace_subdivision_T_x_x_x, T0 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]);
      WrapperReturn( (halfspace_subdivision<T0>(arg0, arg1, arg2)) );
   };

   template <typename T0>
   FunctionInterface4perl( point_collection_T_x_x, T0 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      WrapperReturn( (point_collection<T0>(arg0, arg1)) );
   };

   template <typename T0>
   FunctionInterface4perl( uniform_linear_space_T_x_x_x, T0 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]);
      WrapperReturn( (uniform_linear_space<T0>(arg0, arg1, arg2)) );
   };

   template <typename T0>
   FunctionInterface4perl( empty_cycle_T_x, T0 ) {
      perl::Value arg0(stack[0]);
      WrapperReturn( (empty_cycle<T0>(arg0)) );
   };

   FunctionInstance4perl(empty_cycle_T_x, Min);
   FunctionInstance4perl(uniform_linear_space_T_x_x_x, Max);
   FunctionInstance4perl(uniform_linear_space_T_x_x_x, Min);
   FunctionInstance4perl(halfspace_subdivision_T_x_x_x, Max);
   FunctionInstance4perl(halfspace_subdivision_T_x_x_x, Min);
   FunctionInstance4perl(point_collection_T_x_x, Max);
   FunctionInstance4perl(point_collection_T_x_x, Min);
   FunctionInstance4perl(empty_cycle_T_x, Max);
   FunctionInstance4perl(projective_torus_T_x_x, Max);
   FunctionInstance4perl(projective_torus_T_x_x, Min);
   FunctionInstance4perl(orthant_subdivision_T_x_x_x, Max);
   FunctionInstance4perl(orthant_subdivision_T_x_x_x, Min);
   FunctionInstance4perl(affine_linear_space_T_x_x_x, Max);
   FunctionInstance4perl(affine_linear_space_T_x_x_x, Min);
   FunctionInstance4perl(cross_variety_T_x_x_x_x, Min);
   FunctionInstance4perl(cross_variety_T_x_x_x_x, Max);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
