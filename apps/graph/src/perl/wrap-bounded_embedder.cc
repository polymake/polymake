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

#include "polymake/Matrix.h"
#include "polymake/Rational.h"

namespace polymake { namespace graph { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0>
   FunctionInterface4perl( tentacle_graph_x_X, T0 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      WrapperReturn( tentacle_graph(arg0, arg1.get<T0>()) );
   };

   template <typename T0, typename T1>
   FunctionInterface4perl( bounded_embedder_x_X_x_x_X_x, T0,T1 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]), arg3(stack[3]), arg4(stack[4]), arg5(stack[5]);
      WrapperReturn( bounded_embedder(arg0, arg1.get<T0>(), arg2, arg3, arg4.get<T1>(), arg5) );
   };

   FunctionInstance4perl(tentacle_graph_x_X, perl::Canned< const Matrix< Rational > >);
   FunctionInstance4perl(bounded_embedder_x_X_x_x_X_x, perl::Canned< const Matrix< double > >, perl::Canned< const Matrix< double > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
