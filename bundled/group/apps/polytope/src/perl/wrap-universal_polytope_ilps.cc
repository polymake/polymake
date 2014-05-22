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
#include "polymake/common/boost_dynamic_bitset.h"
#include "polymake/Matrix.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Array.h"
#include "polymake/SparseMatrix.h"
#include "polymake/QuadraticExtension.h"
#include "polymake/Set.h"

namespace polymake { namespace polytope { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1, typename T2, typename T3, typename T4>
   FunctionInterface4perl( simplexity_lower_bound_x_X_X_x_X_o, T0,T1,T2,T3,T4 ) {
      perl::Value arg0(stack[1]), arg1(stack[2]), arg2(stack[3]), arg3(stack[4]), arg4(stack[5]), arg5(stack[6]);
      WrapperReturn( (simplexity_lower_bound<T0,T1>(arg0, arg1.get<T2>(), arg2.get<T3>(), arg3, arg4.get<T4>(), arg5)) );
   };

   template <typename T0, typename T1, typename T2, typename T3>
   FunctionInterface4perl( simplexity_ilp_x_X_X_x_X_o, T0,T1,T2,T3 ) {
      perl::Value arg0(stack[1]), arg1(stack[2]), arg2(stack[3]), arg3(stack[4]), arg4(stack[5]), arg5(stack[6]);
      WrapperReturn( (simplexity_ilp<T0>(arg0, arg1.get<T1>(), arg2.get<T2>(), arg3, arg4.get<T3>(), arg5)) );
   };

   FunctionInstance4perl(simplexity_ilp_x_X_X_x_X_o, Rational, perl::Canned< const Matrix< Rational > >, perl::Canned< const Array< Set< int > > >, perl::Canned< const SparseMatrix< Rational, NonSymmetric > >);
   FunctionInstance4perl(simplexity_lower_bound_x_X_X_x_X_o, Rational, Set< int >, perl::Canned< const Matrix< Rational > >, perl::Canned< const Array< Set< int > > >, perl::Canned< const SparseMatrix< Rational, NonSymmetric > >);
   FunctionInstance4perl(simplexity_lower_bound_x_X_X_x_X_o, Rational, boost_dynamic_bitset, perl::Canned< const Matrix< Rational > >, perl::Canned< const Array< boost_dynamic_bitset > >, perl::Canned< const SparseMatrix< Rational, NonSymmetric > >);
   FunctionInstance4perl(simplexity_lower_bound_x_X_X_x_X_o, QuadraticExtension< Rational >, boost_dynamic_bitset, perl::Canned< const SparseMatrix< QuadraticExtension< Rational >, NonSymmetric > >, perl::Canned< const Array< boost_dynamic_bitset > >, perl::Canned< const SparseMatrix< Rational, NonSymmetric > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
