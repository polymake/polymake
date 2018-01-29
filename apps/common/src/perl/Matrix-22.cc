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

///==== this line controls the automatic file splitting: max.instances=40

#include "polymake/Array.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Integer.h"
#include "polymake/Matrix.h"
#include "polymake/Polynomial.h"
#include "polymake/QuadraticExtension.h"
#include "polymake/Rational.h"
#include "polymake/Set.h"
#include "polymake/SparseMatrix.h"
#include "polymake/client.h"
#include "polymake/linalg.h"

namespace polymake { namespace common { namespace {

///==== Automatically generated contents follow.    Please do not delete this line. ====
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

   OperatorInstance4perl(Binary_mul, perl::Canned< const Wary< Matrix< Rational > > >, perl::Canned< const pm::Transposed<pm::MatrixMinor<pm::Matrix<pm::Rational> const&, pm::Array<int> const&, pm::all_selector const&> > >);
   OperatorInstance4perl(Binary__ora, perl::Canned< const Wary< pm::RepeatedRow<pm::SameElementVector<pm::QuadraticExtension<pm::Rational> const&> > > >, perl::Canned< const Matrix< QuadraticExtension< Rational > > >);
   FunctionInstance4perl(new_X, Matrix< QuadraticExtension< Rational > >, perl::Canned< const pm::ColChain<pm::RepeatedRow<pm::SameElementVector<pm::QuadraticExtension<pm::Rational> const&> > const&, pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&> >);
   OperatorInstance4perl(Binary__ora, perl::Canned< const Wary< pm::RepeatedRow<pm::SameElementVector<pm::QuadraticExtension<pm::Rational> const&> > > >, perl::Canned< const pm::Transposed<pm::MatrixMinor<pm::SparseMatrix<pm::QuadraticExtension<pm::Rational>, pm::NonSymmetric>&, pm::Set<int, pm::operations::cmp> const&, pm::all_selector const&> > >);
   FunctionInstance4perl(new_X, Matrix< QuadraticExtension< Rational > >, perl::Canned< const pm::ColChain<pm::RepeatedRow<pm::SameElementVector<pm::QuadraticExtension<pm::Rational> const&> > const&, pm::Transposed<pm::MatrixMinor<pm::SparseMatrix<pm::QuadraticExtension<pm::Rational>, pm::NonSymmetric>&, pm::Set<int, pm::operations::cmp> const&, pm::all_selector const&> > const&> >);
   OperatorInstance4perl(Binary__ora, perl::Canned< const Wary< Matrix< QuadraticExtension< Rational > > > >, perl::Canned< const pm::Transposed<pm::MatrixMinor<pm::SparseMatrix<pm::QuadraticExtension<pm::Rational>, pm::NonSymmetric>&, pm::Set<int, pm::operations::cmp> const&, pm::all_selector const&> > >);
   FunctionInstance4perl(new_X, Matrix< QuadraticExtension< Rational > >, perl::Canned< const pm::ColChain<pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&, pm::Transposed<pm::MatrixMinor<pm::SparseMatrix<pm::QuadraticExtension<pm::Rational>, pm::NonSymmetric>&, pm::Set<int, pm::operations::cmp> const&, pm::all_selector const&> > const&> >);
   OperatorInstance4perl(Binary__ora, perl::Canned< const Wary< pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::QuadraticExtension<pm::Rational> const&> const&>, pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&> > >, perl::Canned< const Matrix< QuadraticExtension< Rational > > >);
   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const pm::ColChain<pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::QuadraticExtension<pm::Rational> const&> const&>, pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&> const&, pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&> >);
   OperatorInstance4perl(Binary__ora, perl::Canned< const Wary< pm::ColChain<pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::QuadraticExtension<pm::Rational> const&> const&>, pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&> const&, pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&> > >, perl::Canned< const Matrix< QuadraticExtension< Rational > > >);
   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const pm::ColChain<pm::ColChain<pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::QuadraticExtension<pm::Rational> const&> const&>, pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&> const&, pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&> const&, pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&> >);
   OperatorInstance4perl(Binary__ora, perl::Canned< const Wary< pm::ColChain<pm::ColChain<pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::QuadraticExtension<pm::Rational> const&> const&>, pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&> const&, pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&> const&, pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&> > >, perl::Canned< const Matrix< QuadraticExtension< Rational > > >);
   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const pm::ColChain<pm::ColChain<pm::ColChain<pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::QuadraticExtension<pm::Rational> const&> const&>, pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&> const&, pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&> const&, pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&> const&, pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&> >);
   OperatorInstance4perl(Binary__ora, perl::Canned< const Wary< pm::ColChain<pm::ColChain<pm::ColChain<pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::QuadraticExtension<pm::Rational> const&> const&>, pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&> const&, pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&> const&, pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&> const&, pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&> > >, perl::Canned< const Matrix< QuadraticExtension< Rational > > >);
   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const pm::ColChain<pm::ColChain<pm::ColChain<pm::ColChain<pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::QuadraticExtension<pm::Rational> const&> const&>, pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&> const&, pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&> const&, pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&> const&, pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&> const&, pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&> >);
   OperatorInstance4perl(Binary__ora, perl::Canned< const Wary< pm::ColChain<pm::ColChain<pm::ColChain<pm::ColChain<pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::QuadraticExtension<pm::Rational> const&> const&>, pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&> const&, pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&> const&, pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&> const&, pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&> const&, pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&> > >, perl::Canned< const Matrix< QuadraticExtension< Rational > > >);
   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const pm::ColChain<pm::ColChain<pm::ColChain<pm::ColChain<pm::ColChain<pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::QuadraticExtension<pm::Rational> const&> const&>, pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&> const&, pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&> const&, pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&> const&, pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&> const&, pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&> const&, pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&> >);
   FunctionInstance4perl(new_X, Matrix< QuadraticExtension< Rational > >, perl::Canned< const pm::ColChain<pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::QuadraticExtension<pm::Rational> const&> const&>, pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&> const&, pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&> >);
   FunctionInstance4perl(new_X, Matrix< QuadraticExtension< Rational > >, perl::Canned< const pm::ColChain<pm::ColChain<pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::QuadraticExtension<pm::Rational> const&> const&>, pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&> const&, pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&> const&, pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&> >);
   FunctionInstance4perl(new_X, Matrix< QuadraticExtension< Rational > >, perl::Canned< const pm::ColChain<pm::ColChain<pm::ColChain<pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::QuadraticExtension<pm::Rational> const&> const&>, pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&> const&, pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&> const&, pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&> const&, pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&> >);
   FunctionInstance4perl(new_X, Matrix< QuadraticExtension< Rational > >, perl::Canned< const pm::ColChain<pm::ColChain<pm::ColChain<pm::ColChain<pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::QuadraticExtension<pm::Rational> const&> const&>, pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&> const&, pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&> const&, pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&> const&, pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&> const&, pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&> >);
   FunctionInstance4perl(new_X, Matrix< QuadraticExtension< Rational > >, perl::Canned< const pm::MatrixMinor<pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&, pm::all_selector const&, pm::Series<int, true> const&> >);
   FunctionInstance4perl(new_X, Matrix< QuadraticExtension< Rational > >, perl::Canned< const pm::ColChain<pm::ColChain<pm::ColChain<pm::ColChain<pm::ColChain<pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::QuadraticExtension<pm::Rational> const&> const&>, pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&> const&, pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&> const&, pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&> const&, pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&> const&, pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&> const&, pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&> >);
   FunctionInstance4perl(new_X, Matrix< QuadraticExtension< Rational > >, perl::Canned< const pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::QuadraticExtension<pm::Rational> const&> const&>, pm::MatrixMinor<pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&, pm::all_selector const&, pm::Series<int, true> const&> const&> >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Wary< pm::MatrixMinor<pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&, pm::all_selector const&, pm::Series<int, true> const&> > >, perl::Canned< const Matrix< QuadraticExtension< Rational > > >);
   OperatorInstance4perl(BinaryAssign_add, perl::Canned< Wary< Matrix< QuadraticExtension< Rational > > > >, perl::Canned< const Matrix< QuadraticExtension< Rational > > >);
   OperatorInstance4perl(Binary__ora, perl::Canned< const Wary< pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::QuadraticExtension<pm::Rational> const&> const&>, pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&> > >, perl::Canned< const pm::MatrixMinor<pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&, pm::all_selector const&, pm::Series<int, true> const&> >);
   FunctionInstance4perl(new_X, Matrix< QuadraticExtension< Rational > >, perl::Canned< const pm::ColChain<pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::QuadraticExtension<pm::Rational> const&> const&>, pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&> const&, pm::MatrixMinor<pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&, pm::all_selector const&, pm::Series<int, true> const&> const&> >);
   OperatorInstance4perl(Binary__ora, perl::Canned< const Wary< pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::QuadraticExtension<pm::Rational> const&> const&>, pm::MatrixMinor<pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&, pm::all_selector const&, pm::Series<int, true> const&> const&> > >, perl::Canned< const pm::MatrixMinor<pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&, pm::all_selector const&, pm::Series<int, true> const&> >);
   FunctionInstance4perl(new_X, Matrix< QuadraticExtension< Rational > >, perl::Canned< const pm::ColChain<pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::QuadraticExtension<pm::Rational> const&> const&>, pm::MatrixMinor<pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&, pm::all_selector const&, pm::Series<int, true> const&> const&> const&, pm::MatrixMinor<pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&, pm::all_selector const&, pm::Series<int, true> const&> const&> >);
   OperatorInstance4perl(Binary__ora, perl::Canned< const Wary< pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::QuadraticExtension<pm::Rational> const&> const&>, pm::MatrixMinor<pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&, pm::all_selector const&, pm::Series<int, true> const&> const&> > >, perl::Canned< const Matrix< QuadraticExtension< Rational > > >);
   FunctionInstance4perl(new_X, Matrix< QuadraticExtension< Rational > >, perl::Canned< const pm::ColChain<pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::QuadraticExtension<pm::Rational> const&> const&>, pm::MatrixMinor<pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&, pm::all_selector const&, pm::Series<int, true> const&> const&> const&, pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&> >);
   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::QuadraticExtension<pm::Rational> const&> const&>, pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&> >);
   OperatorInstance4perl(assign, Matrix< Rational >, perl::Canned< const Matrix< QuadraticExtension< Rational > > >);
   OperatorInstance4perl(assign, Matrix< QuadraticExtension< Rational > >, perl::Canned< const pm::Transposed<pm::Matrix<pm::QuadraticExtension<pm::Rational> > > >);
   OperatorInstance4perl(assign, Matrix< Rational >, perl::Canned< const pm::Transposed<pm::Matrix<pm::Rational> > >);
   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const pm::RepeatedRow<pm::SameElementVector<pm::Rational const&> > >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Wary< Matrix< Rational > > >, perl::Canned< const pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Integer> const&>, pm::Series<int, true>, mlist<> > >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Wary< Matrix< Rational > > >, perl::Canned< const pm::IndexedSlice<pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Integer> const&>, pm::Series<int, true>, mlist<> > const&, pm::Series<int, true>, mlist<> > >);
   FunctionInstance4perl(new_int_int, Matrix< Polynomial< Rational, int > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
