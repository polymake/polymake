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

namespace polymake { namespace matroid { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1>
   FunctionInterface4perl( valuated_bases_from_circuits_T_x_f16, T0,T1 ) {
      perl::Value arg0(stack[0]);
      WrapperReturnVoid( (valuated_bases_from_circuits<T0,T1>(arg0)) );
   };

   template <typename T0, typename T1>
   FunctionInterface4perl( valuated_circuits_from_bases_T_x_f16, T0,T1 ) {
      perl::Value arg0(stack[0]);
      WrapperReturnVoid( (valuated_circuits_from_bases<T0,T1>(arg0)) );
   };

   FunctionInstance4perl(valuated_circuits_from_bases_T_x_f16, Max, Rational);
   FunctionInstance4perl(valuated_bases_from_circuits_T_x_f16, Max, Rational);
   FunctionInstance4perl(valuated_bases_from_circuits_T_x_f16, Min, Rational);
   FunctionInstance4perl(valuated_circuits_from_bases_T_x_f16, Min, Rational);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
