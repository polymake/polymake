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
#include "polymake/QuadraticExtension.h"

namespace polymake { namespace polytope { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0>
   FunctionInterface4perl( objective_values_for_embedding_x_x, T0 ) {
      perl::Value arg0(stack[1]), arg1(stack[2]);
      WrapperReturn( objective_values_for_embedding<T0>(arg0, arg1) );
   };

   template <typename T0>
   FunctionInterface4perl( dgraph_x_x_o, T0 ) {
      perl::Value arg0(stack[1]), arg1(stack[2]), arg2(stack[3]);
      WrapperReturn( dgraph<T0>(arg0, arg1, arg2) );
   };

   FunctionInstance4perl(dgraph_x_x_o, Rational);
   FunctionInstance4perl(objective_values_for_embedding_x_x, Rational);
   FunctionInstance4perl(dgraph_x_x_o, double);
   FunctionInstance4perl(dgraph_x_x_o, QuadraticExtension< Rational >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
