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
   template <typename T0, typename T1, typename T2, typename T3>
   FunctionInterface4perl( cocircuit_equations_T_x_X_X_X_o, T0,T1,T2,T3 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]), arg3(stack[3]), arg4(stack[4]);
      WrapperReturn( (cocircuit_equations<T0>(arg0, arg1.get<T1>(), arg2.get<T2>(), arg3.get<T3>(), arg4)) );
   };

   template <typename T0, typename T1>
   FunctionInterface4perl( new_X, T0,T1 ) {
      perl::Value arg0(stack[1]);
      WrapperReturnNew(T0, (arg0.get<T1>()) );
   };

   FunctionInstance4perl(new_X, SparseMatrix< Rational, NonSymmetric >, perl::Canned< const pm::ListMatrix<pm::SparseVector<int> > >);
   FunctionInstance4perl(cocircuit_equations_T_x_X_X_X_o, Rational, perl::Canned< const Matrix< Rational > >, perl::Canned< const IncidenceMatrix< NonSymmetric > >, perl::Canned< const Array< Set< int > > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
