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

#include "polymake/PuiseuxFraction.h"
#include "polymake/Rational.h"
#include "polymake/SparseVector.h"
#include "polymake/TropicalNumber.h"

namespace polymake { namespace polytope { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1, typename T2>
   FunctionInterface4perl( goldfarb_sit_T_int_C_C, T0,T1,T2 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]);
      WrapperReturn( (goldfarb_sit<T0>(arg0.get<int>(), arg1.get<T1, T0>(), arg2.get<T2, T0>())) );
   };

   template <typename T0, typename T1, typename T2>
   FunctionInterface4perl( goldfarb_T_int_C_C, T0,T1,T2 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]);
      WrapperReturn( (goldfarb<T0>(arg0.get<int>(), arg1.get<T1, T0>(), arg2.get<T2, T0>())) );
   };

   FunctionInstance4perl(goldfarb_T_int_C_C, Rational, perl::Canned< const Rational >, int);
   FunctionInstance4perl(goldfarb_T_int_C_C, Rational, perl::Canned< const Rational >, perl::Canned< const Rational >);
   FunctionInstance4perl(goldfarb_sit_T_int_C_C, Rational, perl::Canned< const Rational >, perl::Canned< const Rational >);
   FunctionInstance4perl(goldfarb_sit_T_int_C_C, PuiseuxFraction< Min, Rational, Rational >, perl::Canned< const PuiseuxFraction< Min, Rational, Rational > >, perl::Canned< const PuiseuxFraction< Min, Rational, Rational > >);
   FunctionInstance4perl(goldfarb_T_int_C_C, PuiseuxFraction< Min, Rational, Rational >, perl::Canned< const PuiseuxFraction< Min, Rational, Rational > >, int);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
