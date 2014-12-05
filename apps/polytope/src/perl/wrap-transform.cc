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
#include "polymake/Matrix.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Vector.h"
#include "polymake/Set.h"
#include "polymake/QuadraticExtension.h"

namespace polymake { namespace polytope { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1>
   FunctionInterface4perl( scale_T_x_C_x, T0,T1 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]);
      WrapperReturn( (scale<T0>(arg0, arg1.get<T1, T0>(), arg2)) );
   };

   template <typename T0, typename T1>
   FunctionInterface4perl( transform_T_x_X_x, T0,T1 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]);
      WrapperReturn( (transform<T0>(arg0, arg1.get<T1>(), arg2)) );
   };

   template <typename T0, typename T1>
   FunctionInterface4perl( translate_T_x_X_x, T0,T1 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]);
      WrapperReturn( (translate<T0>(arg0, arg1.get<T1>(), arg2)) );
   };

   FunctionInstance4perl(transform_T_x_X_x, Rational, perl::Canned< const Matrix< Rational > >);
   FunctionInstance4perl(translate_T_x_X_x, Rational, perl::Canned< const Vector< Rational > >);
   FunctionInstance4perl(scale_T_x_C_x, Rational, perl::Canned< const Rational >);
   FunctionInstance4perl(scale_T_x_C_x, Rational, int);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
