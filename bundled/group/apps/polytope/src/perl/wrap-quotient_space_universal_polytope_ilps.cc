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
#include "polymake/Array.h"
#include "polymake/Set.h"
#include "polymake/SparseMatrix.h"
#include "polymake/QuadraticExtension.h"

namespace polymake { namespace polytope { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
   FunctionInterface4perl( quotient_space_simplexity_lower_bound_x_X_X_X_X_x_X_X_X_o, T0,T1,T2,T3,T4,T5,T6,T7 ) {
      perl::Value arg0(stack[1]), arg1(stack[2]), arg2(stack[3]), arg3(stack[4]), arg4(stack[5]), arg5(stack[6]), arg6(stack[7]), arg7(stack[8]), arg8(stack[9]), arg9(stack[10]);
      WrapperReturn( (quotient_space_simplexity_lower_bound<T0>(arg0, arg1.get<T1>(), arg2.get<T2>(), arg3.get<T3>(), arg4.get<T4>(), arg5, arg6.get<T5>(), arg7.get<T6>(), arg8.get<T7>(), arg9)) );
   };

   FunctionInstance4perl(quotient_space_simplexity_lower_bound_x_X_X_X_X_x_X_X_X_o, QuadraticExtension< Rational >, perl::Canned< const SparseMatrix< QuadraticExtension< Rational >, NonSymmetric > >, perl::Canned< const IncidenceMatrix< NonSymmetric > >, perl::Canned< const Array< Set< int > > >, perl::Canned< const Array< Set< int > > >, perl::Canned< const SparseMatrix< Rational, NonSymmetric > >, perl::Canned< const Array< Array< int > > >, perl::Canned< const Array< Array< int > > >);
   FunctionInstance4perl(quotient_space_simplexity_lower_bound_x_X_X_X_X_x_X_X_X_o, Rational, perl::Canned< const Matrix< Rational > >, perl::Canned< const IncidenceMatrix< NonSymmetric > >, perl::Canned< const Array< Set< int > > >, perl::Canned< const Array< Set< int > > >, perl::Canned< const SparseMatrix< Rational, NonSymmetric > >, perl::Canned< const Array< Array< int > > >, perl::Canned< const Array< Array< int > > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
