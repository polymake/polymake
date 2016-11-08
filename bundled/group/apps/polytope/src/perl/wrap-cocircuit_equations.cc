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
#include "polymake/common/boost_dynamic_bitset.h"
#include "polymake/hash_map"

namespace polymake { namespace polytope { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1, typename T2, typename T3>
   FunctionInterface4perl( cocircuit_equations_T_x_X_X_o, T0,T1,T2,T3 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]), arg3(stack[3]);
      WrapperReturn( (cocircuit_equations<T0,T1>(arg0, arg1.get<T2>(), arg2.get<T3>(), arg3)) );
   };

   template <typename T0, typename T1, typename T2>
   FunctionInterface4perl( cocircuit_equation_of_ridge_T_x_C, T0,T1,T2 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      WrapperReturn( (cocircuit_equation_of_ridge<T0,T1>(arg0, arg1.get<T2, T1>())) );
   };

   template <typename T0, typename T1, typename T2, typename T3, typename T4, typename T5>
   FunctionInterface4perl( cocircuit_equations_T_x_X_X_X_X_o, T0,T1,T2,T3,T4,T5 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]), arg3(stack[3]), arg4(stack[4]), arg5(stack[5]);
      WrapperReturn( (cocircuit_equations<T0,T1>(arg0, arg1.get<T2>(), arg2.get<T3>(), arg3.get<T4>(), arg4.get<T5>(), arg5)) );
   };

   FunctionInstance4perl(cocircuit_equations_T_x_X_X_X_X_o, Rational, Set< int >, perl::Canned< const Matrix< Rational > >, perl::Canned< const IncidenceMatrix< NonSymmetric > >, perl::Canned< const Array< Set< int > > >, perl::Canned< const Array< Set< int > > >);
   FunctionInstance4perl(cocircuit_equation_of_ridge_T_x_C, Rational, Set< int >, perl::Canned< const Set< int > >);
   FunctionInstance4perl(cocircuit_equation_of_ridge_T_x_C, Rational, boost_dynamic_bitset, perl::Canned< const boost_dynamic_bitset >);
   FunctionInstance4perl(cocircuit_equations_T_x_X_X_o, Rational, Set< int >, perl::Canned< const Array< Set< int > > >, perl::Canned< const Array< Set< int > > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
