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

#include "polymake/Array.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Matrix.h"
#include "polymake/Rational.h"
#include "polymake/Set.h"
#include "polymake/SparseMatrix.h"

namespace polymake { namespace polytope { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8>
   FunctionInterface4perl( simplexity_ilp_with_angles_T_x_X_X_X_X_X_X_C_X, T0,T1,T2,T3,T4,T5,T6,T7,T8 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]), arg3(stack[3]), arg4(stack[4]), arg5(stack[5]), arg6(stack[6]), arg7(stack[7]), arg8(stack[8]);
      WrapperReturn( (simplexity_ilp_with_angles<T0>(arg0, arg1.get<T1>(), arg2.get<T2>(), arg3.get<T3>(), arg4.get<T4>(), arg5.get<T5>(), arg6.get<T6>(), arg7.get<T7, T0>(), arg8.get<T8>())) );
   };

   FunctionInstance4perl(simplexity_ilp_with_angles_T_x_X_X_X_X_X_X_C_X, Rational, perl::Canned< const Matrix< Rational > >, perl::Canned< const Matrix< Rational > >, perl::Canned< const IncidenceMatrix< NonSymmetric > >, perl::Canned< const IncidenceMatrix< NonSymmetric > >, perl::Canned< const Array< Array< int > > >, perl::Canned< const Array< Set< int > > >, perl::Canned< const Rational >, perl::Canned< const SparseMatrix< Rational, NonSymmetric > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
