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

///==== this line controls the automatic file splitting: max.instances=20

#include "polymake/client.h"
#include "polymake/Matrix.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/linalg.h"
#include "polymake/SparseMatrix.h"
#include "polymake/Integer.h"
#include "polymake/QuadraticExtension.h"
#include "polymake/Rational.h"
#include "polymake/Set.h"

namespace polymake { namespace common { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0>
   FunctionInterface4perl( new, T0 ) {
      WrapperReturnNew(T0, () );
   };

   template <typename T0>
   FunctionInterface4perl( new_int_int, T0 ) {
      perl::Value arg0(stack[1]), arg1(stack[2]);
      WrapperReturnNew(T0, (arg0.get<int>(), arg1.get<int>()) );
   };

   template <typename T0, typename T1>
   FunctionInterface4perl( new_X, T0,T1 ) {
      perl::Value arg0(stack[1]);
      WrapperReturnNew(T0, (arg0.get<T1>()) );
   };

   FunctionInstance4perl(new_X, Matrix< int >, perl::Canned< const pm::DiagMatrix<pm::SameElementVector<int const&>, true> >);
   FunctionInstance4perl(new_X, Matrix< int >, perl::Canned< const SparseMatrix< int, NonSymmetric > >);
   FunctionInstance4perl(new_int_int, Matrix< Integer >);
   Class4perl("Polymake::common::Matrix_A_QuadraticExtension__Rational_I_NonSymmetric_Z", Matrix< QuadraticExtension< Rational > >);
   FunctionInstance4perl(new, Matrix< QuadraticExtension< Rational > >);
   OperatorInstance4perl(convert, Matrix< QuadraticExtension< Rational > >, perl::Canned< const SparseMatrix< QuadraticExtension< Rational >, NonSymmetric > >);
   FunctionInstance4perl(new_X, Matrix< QuadraticExtension< Rational > >, perl::Canned< const Matrix< QuadraticExtension< Rational > > >);
   FunctionInstance4perl(new_int_int, Matrix< QuadraticExtension< Rational > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Wary< Matrix< QuadraticExtension< Rational > > > >, perl::Canned< const Matrix< QuadraticExtension< Rational > > >);
   OperatorInstance4perl(convert, Matrix< Rational >, perl::Canned< const SparseMatrix< QuadraticExtension< Rational >, NonSymmetric > >);
   FunctionInstance4perl(new_X, Matrix< QuadraticExtension< Rational > >, perl::Canned< const Matrix< Rational > >);
   OperatorInstance4perl(convert, Matrix< Rational >, perl::Canned< const Matrix< QuadraticExtension< Rational > > >);
   FunctionInstance4perl(new_X, Matrix< int >, perl::Canned< const Matrix< Integer > >);
   OperatorInstance4perl(Binary_div, perl::Canned< const Wary< Matrix< Rational > > >, int);
   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const Matrix< QuadraticExtension< Rational > > >);
   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const pm::RowChain<pm::SparseMatrix<pm::QuadraticExtension<pm::Rational>, pm::NonSymmetric> const&, pm::MatrixMinor<pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&, pm::Set<int, pm::operations::cmp> const&, pm::Series<int, true> const&> const&> >);
   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const pm::MatrixMinor<pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&, pm::all_selector const&, pm::Series<int, true> const&> >);
   FunctionInstance4perl(new_X, Matrix< QuadraticExtension< Rational > >, perl::Canned< const pm::MatrixMinor<pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&, pm::Complement<pm::Set<int, pm::operations::cmp>, int, pm::operations::cmp> const&, pm::Series<int, true> const&> >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
