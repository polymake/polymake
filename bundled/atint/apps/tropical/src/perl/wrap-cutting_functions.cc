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

#include "polymake/Integer.h"
#include "polymake/Matrix.h"
#include "polymake/TropicalNumber.h"
#include "polymake/Vector.h"

namespace polymake { namespace tropical { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1>
   FunctionInterface4perl( cutting_functions_T_x_X, T0,T1 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      WrapperReturn( (cutting_functions<T0>(arg0, arg1.get<T1>())) );
   };

   FunctionWrapper4perl( pm::Matrix<pm::Rational> (perl::Object, pm::Vector<pm::Integer>) ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      IndirectWrapperReturn( arg0, arg1.get< perl::TryCanned< const Vector< Integer > > >() );
   }
   FunctionWrapperInstance4perl( pm::Matrix<pm::Rational> (perl::Object, pm::Vector<pm::Integer>) );

   FunctionInstance4perl(cutting_functions_T_x_X, Max, perl::Canned< const Vector< Integer > >);
   FunctionInstance4perl(cutting_functions_T_x_X, Min, perl::Canned< const Vector< Integer > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
