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
#include "polymake/Vector.h"
#include "polymake/linalg.h"

namespace polymake { namespace polytope { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1>
   FunctionInterface4perl( polytope_contains_point_x_X, T0,T1 ) {
      perl::Value arg0(stack[1]), arg1(stack[2]);
      WrapperReturn( polytope_contains_point<T0>(arg0, arg1.get<T1>()) );
   };

   template <typename T0>
   FunctionInterface4perl( cdd_solve_lp_x_x_x_f16, T0 ) {
      perl::Value arg0(stack[1]), arg1(stack[2]), arg2(stack[3]);
      WrapperReturnVoid( cdd_solve_lp<T0>(arg0, arg1, arg2) );
   };

   template <typename T0>
   FunctionInterface4perl( cdd_input_feasible_x, T0 ) {
      perl::Value arg0(stack[1]);
      WrapperReturn( cdd_input_feasible<T0>(arg0) );
   };

   template <typename T0>
   FunctionInterface4perl( cdd_input_bounded_x, T0 ) {
      perl::Value arg0(stack[1]);
      WrapperReturn( cdd_input_bounded<T0>(arg0) );
   };

   FunctionInstance4perl(cdd_input_bounded_x, Rational);
   FunctionInstance4perl(cdd_input_feasible_x, Rational);
   FunctionInstance4perl(cdd_solve_lp_x_x_x_f16, Rational);
   FunctionInstance4perl(cdd_solve_lp_x_x_x_f16, double);
   FunctionInstance4perl(cdd_input_bounded_x, double);
   FunctionInstance4perl(polytope_contains_point_x_X, Rational, perl::Canned< const Vector< Rational > >);
   FunctionInstance4perl(cdd_input_feasible_x, double);
   FunctionInstance4perl(polytope_contains_point_x_X, Rational, perl::Canned< const pm::SameElementSparseVector<pm::SingleElementSet<int>, pm::Rational> >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
