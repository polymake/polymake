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

#include "polymake/Rational.h"
#include "polymake/TropicalNumber.h"

namespace polymake { namespace tropical { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1, typename T2>
   FunctionInterface4perl( matroid_polytope_A_T_x_C, T0,T1,T2 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      WrapperReturn( (matroid_polytope<T0,T1>(arg0, arg1.get<T2, T1>())) );
   };

   FunctionWrapper4perl( perl::Object (perl::Object, pm::Rational const&) ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      IndirectWrapperReturn( arg0, arg1.get< perl::TryCanned< const Rational > >() );
   }
   FunctionWrapperInstance4perl( perl::Object (perl::Object, pm::Rational const&) );

   FunctionCrossAppInstance4perl(matroid_polytope_A_T_x_C, (1, "matroid"), Min, Rational, int);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
