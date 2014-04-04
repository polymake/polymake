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
*/

#include "polymake/Rational.h"
#include "polymake/QuadraticExtension.h"

namespace polymake { namespace polytope { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1>
   FunctionInterface4perl( simplex_int_X_o, T0,T1 ) {
      perl::Value arg0(stack[1]), arg1(stack[2]), arg2(stack[3]);
      WrapperReturn( (simplex<T0>(arg0.get<int>(), arg1.get<T1>(), arg2)) );
   };

   FunctionInstance4perl(simplex_int_X_o, QuadraticExtension< Rational >, perl::Canned< const QuadraticExtension< Rational > >);
   FunctionInstance4perl(simplex_int_X_o, Rational, int);
   FunctionInstance4perl(simplex_int_X_o, Rational, perl::Canned< const Rational >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
