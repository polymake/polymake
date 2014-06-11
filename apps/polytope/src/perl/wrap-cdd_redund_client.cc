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

#include "polymake/Rational.h"

namespace polymake { namespace polytope { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0>
   FunctionInterface4perl( cdd_canonicalize_lineality_x_x_f16, T0 ) {
      perl::Value arg0(stack[1]), arg1(stack[2]);
      WrapperReturnVoid( cdd_canonicalize_lineality<T0>(arg0, arg1) );
   };

   template <typename T0>
   FunctionInterface4perl( cdd_eliminate_redundant_points_x_f16, T0 ) {
      perl::Value arg0(stack[1]);
      WrapperReturnVoid( cdd_eliminate_redundant_points<T0>(arg0) );
   };

   template <typename T0>
   FunctionInterface4perl( cdd_vertex_normals_x_f16, T0 ) {
      perl::Value arg0(stack[1]);
      WrapperReturnVoid( cdd_vertex_normals<T0>(arg0) );
   };

   template <typename T0>
   FunctionInterface4perl( cdd_canonicalize_x_x_f16, T0 ) {
      perl::Value arg0(stack[1]), arg1(stack[2]);
      WrapperReturnVoid( cdd_canonicalize<T0>(arg0, arg1) );
   };

   FunctionInstance4perl(cdd_canonicalize_x_x_f16, double);
   FunctionInstance4perl(cdd_vertex_normals_x_f16, double);
   FunctionInstance4perl(cdd_eliminate_redundant_points_x_f16, double);
   FunctionInstance4perl(cdd_eliminate_redundant_points_x_f16, Rational);
   FunctionInstance4perl(cdd_canonicalize_x_x_f16, Rational);
   FunctionInstance4perl(cdd_vertex_normals_x_f16, Rational);
   FunctionInstance4perl(cdd_canonicalize_lineality_x_x_f16, Rational);
   FunctionInstance4perl(cdd_canonicalize_lineality_x_x_f16, double);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }