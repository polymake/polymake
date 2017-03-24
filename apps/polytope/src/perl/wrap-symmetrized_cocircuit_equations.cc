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

#include "polymake/Array.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Matrix.h"
#include "polymake/Rational.h"
#include "polymake/Set.h"
#include "polymake/SparseMatrix.h"
#include "polymake/Bitset.h"

namespace polymake { namespace polytope { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1, typename T2, typename T3, typename T4, typename T5>
   FunctionInterface4perl( cocircuit_equations_support_reps_T_X_X_X_X_o, T0,T1,T2,T3,T4,T5 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]), arg3(stack[3]), arg4(stack[4]);
      WrapperReturn( (cocircuit_equations_support_reps<T0,T1>(arg0.get<T2>(), arg1.get<T3>(), arg2.get<T4>(), arg3.get<T5>(), arg4)) );
   };

   template <typename T0, typename T1, typename T2, typename T3, typename T4>
   FunctionInterface4perl( combinatorial_symmetrized_cocircuit_equations_T_x_X_X_X_o, T0,T1,T2,T3,T4 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]), arg3(stack[3]), arg4(stack[4]);
      WrapperReturn( (combinatorial_symmetrized_cocircuit_equations<T0,T1>(arg0, arg1.get<T2>(), arg2.get<T3>(), arg3.get<T4>(), arg4)) );
   };

   template <typename T0, typename T1, typename T2, typename T3, typename T4>
   FunctionInterface4perl( projected_symmetrized_cocircuit_equations_impl_T_x_X_X_X_x, T0,T1,T2,T3,T4 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]), arg3(stack[3]), arg4(stack[4]);
      WrapperReturn( (projected_symmetrized_cocircuit_equations_impl<T0,T1>(arg0, arg1.get<T2>(), arg2.get<T3>(), arg3.get<T4>(), arg4)) );
   };

   template <typename T0, typename T1>
   FunctionInterface4perl( new_X, T0,T1 ) {
      perl::Value arg0(stack[1]);
      WrapperReturnNew(T0, (arg0.get<T1>()) );
   };

   template <typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
   FunctionInterface4perl( symmetrized_cocircuit_equations_0_T_x_X_X_X_X_X_o, T0,T1,T2,T3,T4,T5,T6 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]), arg3(stack[3]), arg4(stack[4]), arg5(stack[5]), arg6(stack[6]);
      WrapperReturn( (symmetrized_cocircuit_equations_0<T0,T1>(arg0, arg1.get<T2>(), arg2.get<T3>(), arg3.get<T4>(), arg4.get<T5>(), arg5.get<T6>(), arg6)) );
   };

   FunctionInstance4perl(symmetrized_cocircuit_equations_0_T_x_X_X_X_X_X_o, Rational, Set< int >, perl::Canned< const Matrix< Rational > >, perl::Canned< const IncidenceMatrix< NonSymmetric > >, perl::Canned< const Array< Array< int > > >, perl::Canned< const Array< Set< int > > >, perl::Canned< const Array< Set< int > > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Wary< SparseMatrix< int, NonSymmetric > > >, perl::Canned< const pm::ListMatrix<pm::SparseVector<int> > >);
   FunctionInstance4perl(new_X, SparseMatrix< Rational, NonSymmetric >, perl::Canned< const pm::ListMatrix<pm::SparseVector<int> > >);
   FunctionInstance4perl(projected_symmetrized_cocircuit_equations_impl_T_x_X_X_X_x, Rational, Bitset, perl::Canned< const Array< Bitset > >, perl::Canned< const Array< Bitset > >, perl::Canned< const pm::SingleElementSet<int> >);
   FunctionInstance4perl(projected_symmetrized_cocircuit_equations_impl_T_x_X_X_X_x, Rational, Bitset, perl::Canned< const Array< Bitset > >, perl::Canned< const Array< Bitset > >, perl::Canned< const Set< int > >);
   FunctionInstance4perl(combinatorial_symmetrized_cocircuit_equations_T_x_X_X_X_o, Rational, Bitset, perl::Canned< const Array< Bitset > >, perl::Canned< const Array< Bitset > >, perl::Canned< const pm::SingleElementSet<int> >);
   FunctionInstance4perl(combinatorial_symmetrized_cocircuit_equations_T_x_X_X_X_o, Rational, Bitset, perl::Canned< const Array< Bitset > >, perl::Canned< const Array< Bitset > >, perl::Canned< const Set< int > >);
   FunctionInstance4perl(cocircuit_equations_support_reps_T_X_X_X_X_o, Rational, Bitset, perl::Canned< const Matrix< Rational > >, perl::Canned< const Array< Array< int > > >, perl::Canned< const Array< Bitset > >, perl::Canned< const Array< Bitset > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
