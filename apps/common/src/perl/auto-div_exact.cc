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

///==== this line controls the automatic file splitting: max.instances=40

#include "polymake/client.h"
#include "polymake/Matrix.h"
#include "polymake/Integer.h"
#include "polymake/Vector.h"
#include "polymake/SparseMatrix.h"
#include "polymake/IncidenceMatrix.h"

namespace polymake { namespace common { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1>
   FunctionInterface4perl( div_exact_X_X, T0,T1 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      WrapperReturn( div_exact(arg0.get<T0>(), arg1.get<T1>()) );
   };

   template <typename T0, typename T1>
   FunctionInterface4perl( div_exact_X_f3, T0,T1 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      WrapperReturnLvalue( T0, arg0.get<T0>().div_exact(arg1.get<T1>()) );
   };

   FunctionInstance4perl(div_exact_X_X, perl::Canned< const pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Integer>&>, pm::Series<int, true>, void> >, perl::Canned< const Integer >);
   FunctionInstance4perl(div_exact_X_f3, perl::Canned< Vector< Integer > >, perl::Canned< const Integer >);
   FunctionInstance4perl(div_exact_X_f3, perl::Canned< pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Integer>&>, pm::Series<int, true>, void> >, perl::Canned< const Integer >);
   FunctionInstance4perl(div_exact_X_X, perl::Canned< const Vector< Integer > >, perl::Canned< const Integer >);
   FunctionInstance4perl(div_exact_X_X, perl::Canned< const pm::sparse_matrix_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::Integer, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> >&, pm::NonSymmetric> >, perl::Canned< const Integer >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
