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

#include "polymake/IncidenceMatrix.h"
#include "polymake/Matrix.h"
#include "polymake/QuadraticExtension.h"
#include "polymake/Rational.h"
#include "polymake/SparseMatrix.h"

namespace polymake { namespace polytope { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1, typename T2, typename T3, typename T4>
   FunctionInterface4perl( minkowski_sum_client_T_C_X_C_X, T0,T1,T2,T3,T4 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]), arg3(stack[3]);
      WrapperReturn( (minkowski_sum_client<T0>(arg0.get<T1, T0>(), arg1.get<T2>(), arg2.get<T3, T0>(), arg3.get<T4>())) );
   };

   FunctionInstance4perl(minkowski_sum_client_T_C_X_C_X, Rational, int, perl::Canned< const Matrix< Rational > >, int, perl::Canned< const Matrix< Rational > >);
   FunctionInstance4perl(minkowski_sum_client_T_C_X_C_X, double, int, perl::Canned< const Matrix< double > >, int, perl::Canned< const Matrix< double > >);
   FunctionInstance4perl(minkowski_sum_client_T_C_X_C_X, QuadraticExtension< Rational >, int, perl::Canned< const Matrix< QuadraticExtension< Rational > > >, int, perl::Canned< const Matrix< QuadraticExtension< Rational > > >);
   FunctionInstance4perl(minkowski_sum_client_T_C_X_C_X, Rational, int, perl::Canned< const SparseMatrix< Rational, NonSymmetric > >, int, perl::Canned< const SparseMatrix< Rational, NonSymmetric > >);
   FunctionInstance4perl(minkowski_sum_client_T_C_X_C_X, Rational, int, perl::Canned< const Matrix< Rational > >, int, perl::Canned< const SparseMatrix< Rational, NonSymmetric > >);
   FunctionInstance4perl(minkowski_sum_client_T_C_X_C_X, Rational, int, perl::Canned< const SparseMatrix< Rational, NonSymmetric > >, int, perl::Canned< const Matrix< Rational > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
