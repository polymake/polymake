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

#include "polymake/Matrix.h"
#include "polymake/Rational.h"
#include "polymake/TropicalNumber.h"
#include "polymake/Vector.h"

namespace polymake { namespace tropical { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1>
   FunctionInterface4perl( hurwitz_pair_local_T_x_X_x_o, T0,T1 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]), arg3(stack[3]);
      WrapperReturn( (hurwitz_pair_local<T0>(arg0, arg1.get<T1>(), arg2, arg3)) );
   };

   template <typename T0, typename T1, typename T2>
   FunctionInterface4perl( hurwitz_cycle_T_x_X_X_o, T0,T1,T2 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]), arg3(stack[3]);
      WrapperReturn( (hurwitz_cycle<T0>(arg0, arg1.get<T1>(), arg2.get<T2>(), arg3)) );
   };

   template <typename T0, typename T1, typename T2>
   FunctionInterface4perl( hurwitz_subdivision_T_x_X_X_o, T0,T1,T2 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]), arg3(stack[3]);
      WrapperReturn( (hurwitz_subdivision<T0>(arg0, arg1.get<T1>(), arg2.get<T2>(), arg3)) );
   };

   FunctionWrapper4perl( perl::Object (perl::Object, pm::Vector<int>) ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      IndirectWrapperReturn( arg0, arg1.get< perl::TryCanned< const Vector< int > > >() );
   }
   FunctionWrapperInstance4perl( perl::Object (perl::Object, pm::Vector<int>) );

   FunctionInstance4perl(hurwitz_subdivision_T_x_X_X_o, Max, perl::Canned< const Vector< int > >, perl::Canned< const Vector< Rational > >);
   FunctionInstance4perl(hurwitz_subdivision_T_x_X_X_o, Min, perl::Canned< const Vector< int > >, perl::Canned< const Vector< Rational > >);
   FunctionInstance4perl(hurwitz_cycle_T_x_X_X_o, Max, perl::Canned< const Vector< int > >, perl::Canned< const Vector< Rational > >);
   FunctionInstance4perl(hurwitz_cycle_T_x_X_X_o, Min, perl::Canned< const Vector< int > >, perl::Canned< const Vector< Rational > >);
   FunctionInstance4perl(hurwitz_pair_local_T_x_X_x_o, Max, perl::Canned< const Vector< int > >);
   FunctionInstance4perl(hurwitz_pair_local_T_x_X_x_o, Min, perl::Canned< const Vector< int > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
