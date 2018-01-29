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
#include "polymake/Bitset.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Matrix.h"
#include "polymake/QuadraticExtension.h"
#include "polymake/Rational.h"
#include "polymake/Set.h"
#include "polymake/SparseMatrix.h"

namespace polymake { namespace polytope { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1, typename T2, typename T3, typename T4>
   FunctionInterface4perl( simplexity_ilp_T_x_X_X_x_X, T0,T1,T2,T3,T4 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]), arg3(stack[3]), arg4(stack[4]);
      WrapperReturn( (simplexity_ilp<T0,T1>(arg0, arg1.get<T2>(), arg2.get<T3>(), arg3, arg4.get<T4>())) );
   };

   template <typename T0, typename T1, typename T2, typename T3>
   FunctionInterface4perl( universal_polytope_impl_T_x_X_X_x_X, T0,T1,T2,T3 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]), arg3(stack[3]), arg4(stack[4]);
      WrapperReturn( (universal_polytope_impl<T0>(arg0, arg1.get<T1>(), arg2.get<T2>(), arg3, arg4.get<T3>())) );
   };

   template <typename T0, typename T1, typename T2, typename T3, typename T4>
   FunctionInterface4perl( foldable_max_signature_ilp_T_x_X_X_x_C, T0,T1,T2,T3,T4 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]), arg3(stack[3]), arg4(stack[4]);
      WrapperReturn( (foldable_max_signature_ilp<T0,T1>(arg0, arg1.get<T2>(), arg2.get<T3>(), arg3, arg4.get<T4, T1>())) );
   };

   template <typename T0, typename T1, typename T2, typename T3, typename T4>
   FunctionInterface4perl( simplexity_lower_bound_T_x_X_X_x_X, T0,T1,T2,T3,T4 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]), arg3(stack[3]), arg4(stack[4]);
      WrapperReturn( (simplexity_lower_bound<T0,T1>(arg0, arg1.get<T2>(), arg2.get<T3>(), arg3, arg4.get<T4>())) );
   };

   template <typename T0, typename T1, typename T2, typename T3>
   FunctionInterface4perl( foldable_max_signature_upper_bound_T_x_X_X_x_X, T0,T1,T2,T3 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]), arg3(stack[3]), arg4(stack[4]);
      WrapperReturn( (foldable_max_signature_upper_bound<T0>(arg0, arg1.get<T1>(), arg2.get<T2>(), arg3, arg4.get<T3>())) );
   };

   FunctionInstance4perl(universal_polytope_impl_T_x_X_X_x_X, Rational, perl::Canned< const Matrix< Rational > >, perl::Canned< const Array< Set< int > > >, perl::Canned< const SparseMatrix< Rational, NonSymmetric > >);
   FunctionInstance4perl(foldable_max_signature_ilp_T_x_X_X_x_C, Set< int >, SparseMatrix< Rational, NonSymmetric >, perl::Canned< const Matrix< Rational > >, perl::Canned< const Array< Set< int > > >, perl::Canned< const SparseMatrix< Rational, NonSymmetric > >);
   FunctionInstance4perl(simplexity_lower_bound_T_x_X_X_x_X, Rational, Set< int >, perl::Canned< const Matrix< Rational > >, perl::Canned< const Array< Set< int > > >, perl::Canned< const SparseMatrix< Rational, NonSymmetric > >);
   FunctionInstance4perl(foldable_max_signature_upper_bound_T_x_X_X_x_X, Set< int >, perl::Canned< const Matrix< Rational > >, perl::Canned< const Array< Set< int > > >, perl::Canned< const SparseMatrix< Rational, NonSymmetric > >);
   FunctionInstance4perl(simplexity_lower_bound_T_x_X_X_x_X, Rational, Bitset, perl::Canned< const Matrix< Rational > >, perl::Canned< const Array< Bitset > >, perl::Canned< const SparseMatrix< Rational, NonSymmetric > >);
   FunctionInstance4perl(simplexity_lower_bound_T_x_X_X_x_X, QuadraticExtension< Rational >, Bitset, perl::Canned< const SparseMatrix< QuadraticExtension< Rational >, NonSymmetric > >, perl::Canned< const Array< Bitset > >, perl::Canned< const SparseMatrix< Rational, NonSymmetric > >);
   FunctionInstance4perl(simplexity_ilp_T_x_X_X_x_X, Rational, Set< int >, perl::Canned< const Matrix< Rational > >, perl::Canned< const Array< Set< int > > >, perl::Canned< const SparseMatrix< Rational, NonSymmetric > >);
   FunctionInstance4perl(simplexity_ilp_T_x_X_X_x_X, Rational, Bitset, perl::Canned< const Matrix< Rational > >, perl::Canned< const Array< Bitset > >, perl::Canned< const SparseMatrix< Rational, NonSymmetric > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
