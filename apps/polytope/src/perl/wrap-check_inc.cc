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
#include "polymake/SparseMatrix.h"

namespace polymake { namespace polytope { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1, typename T2>
   FunctionInterface4perl( check_inc_T_X_X_x_x, T0,T1,T2 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]), arg3(stack[3]);
      WrapperReturn( (check_inc<T0>(arg0.get<T1>(), arg1.get<T2>(), arg2, arg3)) );
   };

   FunctionInstance4perl(check_inc_T_X_X_x_x, Rational, perl::Canned< const Matrix< Rational > >, perl::Canned< const Matrix< Rational > >);
   FunctionInstance4perl(check_inc_T_X_X_x_x, Rational, perl::Canned< const Matrix< Rational > >, perl::Canned< const pm::MatrixMinor<pm::SparseMatrix<pm::Rational, pm::NonSymmetric> const&, pm::Array<int> const&, pm::all_selector const&> >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
