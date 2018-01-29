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

#include "polymake/Matrix.h"
#include "polymake/Rational.h"
#include "polymake/TropicalNumber.h"
#include "polymake/Vector.h"

namespace polymake { namespace tropical { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0>
   FunctionInterface4perl( matroid_coordinates_from_curve_T_x, T0 ) {
      perl::Value arg0(stack[0]);
      WrapperReturn( (matroid_coordinates_from_curve<T0>(arg0)) );
   };

   template <typename T0, typename T1>
   FunctionInterface4perl( matroid_vector_T_x_C, T0,T1 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      WrapperReturn( (matroid_vector<T0>(arg0, arg1.get<T1, T0>())) );
   };

   template <typename T0, typename T1>
   FunctionInterface4perl( rational_curve_from_matroid_coordinates_T_X, T0,T1 ) {
      perl::Value arg0(stack[0]);
      WrapperReturn( (rational_curve_from_matroid_coordinates<T0>(arg0.get<T1>())) );
   };

   FunctionWrapper4perl( pm::Vector<pm::Rational> (pm::IncidenceMatrix<pm::NonSymmetric>, pm::Vector<pm::Rational>, int) ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]);
      IndirectWrapperReturn( arg0.get< perl::TryCanned< const IncidenceMatrix< NonSymmetric > > >(), arg1.get< perl::TryCanned< const Vector< Rational > > >(), arg2 );
   }
   FunctionWrapperInstance4perl( pm::Vector<pm::Rational> (pm::IncidenceMatrix<pm::NonSymmetric>, pm::Vector<pm::Rational>, int) );

   FunctionWrapper4perl( pm::perl::ListReturn (pm::Vector<pm::Rational>) ) {
      perl::Value arg0(stack[0]);
      IndirectWrapperReturnVoid( arg0.get< perl::TryCanned< const Vector< Rational > > >() );
   }
   FunctionWrapperInstance4perl( pm::perl::ListReturn (pm::Vector<pm::Rational>) );

   FunctionInstance4perl(rational_curve_from_matroid_coordinates_T_X, Max, perl::Canned< const Vector< Rational > >);
   FunctionInstance4perl(rational_curve_from_matroid_coordinates_T_X, Min, perl::Canned< const Vector< Rational > >);
   FunctionInstance4perl(matroid_vector_T_x_C, Max, perl::Canned< const Max >);
   FunctionInstance4perl(matroid_vector_T_x_C, Min, perl::Canned< const Min >);
   FunctionInstance4perl(matroid_coordinates_from_curve_T_x, Max);
   FunctionInstance4perl(matroid_coordinates_from_curve_T_x, Min);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
