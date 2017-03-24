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
#include "polymake/Matrix.h"
#include "polymake/PuiseuxFraction.h"
#include "polymake/QuadraticExtension.h"
#include "polymake/Rational.h"
#include "polymake/SparseMatrix.h"
#include "polymake/TropicalNumber.h"

namespace polymake { namespace polytope { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0>
   FunctionInterface4perl( placing_triangulation_X_o, T0 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      WrapperReturn( (placing_triangulation(arg0.get<T0>(), arg1)) );
   };

   template <typename T0>
   FunctionInterface4perl( beneath_beyond_T_x_x_x_f16, T0 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]);
      WrapperReturnVoid( (beneath_beyond<T0>(arg0, arg1, arg2)) );
   };

   FunctionInstance4perl(beneath_beyond_T_x_x_x_f16, Rational);
   FunctionInstance4perl(beneath_beyond_T_x_x_x_f16, QuadraticExtension< Rational >);
   FunctionInstance4perl(placing_triangulation_X_o, perl::Canned< const Matrix< Rational > >);
   FunctionInstance4perl(placing_triangulation_X_o, perl::Canned< const SparseMatrix< Rational, NonSymmetric > >);
   FunctionInstance4perl(beneath_beyond_T_x_x_x_f16, PuiseuxFraction< Min, Rational, Rational >);
   FunctionInstance4perl(placing_triangulation_X_o, perl::Canned< const Matrix< PuiseuxFraction< Min, Rational, Rational > > >);
   FunctionInstance4perl(beneath_beyond_T_x_x_x_f16, PuiseuxFraction< Max, Rational, Rational >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
