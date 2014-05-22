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
#include "polymake/Matrix.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/SparseMatrix.h"

namespace polymake { namespace polytope { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1, typename T2>
   FunctionInterface4perl( minkowski_sum_client_x_X_x_X, T0,T1,T2 ) {
      perl::Value arg0(stack[1]), arg1(stack[2]), arg2(stack[3]), arg3(stack[4]);
      WrapperReturn( (minkowski_sum_client<T0>(arg0, arg1.get<T1>(), arg2, arg3.get<T2>())) );
   };

   FunctionInstance4perl(minkowski_sum_client_x_X_x_X, Rational, perl::Canned< const SparseMatrix< Rational, NonSymmetric > >, perl::Canned< const Matrix< Rational > >);
   FunctionInstance4perl(minkowski_sum_client_x_X_x_X, Rational, perl::Canned< const Matrix< Rational > >, perl::Canned< const Matrix< Rational > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
