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

#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Array.h"
#include "polymake/QuadraticExtension.h"
#include "polymake/SparseMatrix.h"

namespace polymake { namespace polytope { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0>
   FunctionInterface4perl( representative_interior_and_boundary_ridges_T_x_o, T0 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      WrapperReturn( (representative_interior_and_boundary_ridges<T0>(arg0, arg1)) );
   };

   template <typename T0, typename T1, typename T2>
   FunctionInterface4perl( representative_max_interior_simplices_T_x_X_X, T0,T1,T2 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]);
      WrapperReturn( (representative_max_interior_simplices<T0>(arg0, arg1.get<T1>(), arg2.get<T2>())) );
   };

   template <typename T0, typename T1, typename T2>
   FunctionInterface4perl( representative_simplices_T_x_X_X, T0,T1,T2 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]);
      WrapperReturn( (representative_simplices<T0>(arg0, arg1.get<T1>(), arg2.get<T2>())) );
   };

   FunctionInstance4perl(representative_interior_and_boundary_ridges_T_x_o, Rational);
   FunctionInstance4perl(representative_max_interior_simplices_T_x_X_X, Rational, perl::Canned< const Matrix< Rational > >, perl::Canned< const Array< Array< int > > >);
   FunctionInstance4perl(representative_simplices_T_x_X_X, Rational, perl::Canned< const Matrix< Rational > >, perl::Canned< const Array< Array< int > > >);
   FunctionInstance4perl(representative_simplices_T_x_X_X, QuadraticExtension< Rational >, perl::Canned< const SparseMatrix< QuadraticExtension< Rational >, NonSymmetric > >, perl::Canned< const Array< Array< int > > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
