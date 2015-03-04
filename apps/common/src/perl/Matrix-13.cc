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

///==== this line controls the automatic file splitting: max.instances=20

#include "polymake/client.h"
#include "polymake/Matrix.h"
#include "polymake/Rational.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Set.h"
#include "polymake/Integer.h"
#include "polymake/linalg.h"
#include "polymake/Vector.h"
#include "polymake/Array.h"
#include "polymake/QuadraticExtension.h"

namespace polymake { namespace common { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1>
   FunctionInterface4perl( new_X, T0,T1 ) {
      perl::Value arg0(stack[1]);
      WrapperReturnNew(T0, (arg0.get<T1>()) );
   };

   FunctionInstance4perl(new_X, Matrix< double >, perl::Canned< const pm::RowChain<pm::Matrix<double> const&, pm::Matrix<double> const&> >);
   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const pm::ColChain<pm::SingleCol<pm::Vector<int> const&>, pm::MatrixMinor<pm::Matrix<int> const&, pm::Complement<pm::Set<int, pm::operations::cmp>, int, pm::operations::cmp> const&, pm::all_selector const&> const&> >);
   OperatorInstance4perl(Binary_diva, perl::Canned< const Wary< Matrix< QuadraticExtension< Rational > > > >, perl::Canned< const Matrix< QuadraticExtension< Rational > > >);
   OperatorInstance4perl(convert, Matrix< double >, perl::Canned< const Matrix< QuadraticExtension< Rational > > >);
   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::Rational const&> const&>, pm::MatrixMinor<pm::Matrix<pm::Rational> const&, pm::Array<int, void> const&, pm::all_selector const&> const&> >);
   OperatorInstance4perl(Binary_diva, perl::Canned< const Wary< pm::RowChain<pm::Matrix<pm::Integer> const&, pm::Matrix<pm::Integer> const&> > >, perl::Canned< const Matrix< Integer > >);
   OperatorInstance4perl(assign, Matrix< Integer >, perl::Canned< const pm::DiagMatrix<pm::SameElementVector<pm::Integer const&>, true> >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Wary< Matrix< Integer > > >, perl::Canned< const pm::RowChain<pm::DiagMatrix<pm::SameElementVector<pm::Rational const&>, true> const&, pm::RepeatedRow<pm::SameElementVector<pm::Rational const&> > const&> >);
   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const pm::MatrixMinor<pm::ColChain<pm::DiagMatrix<pm::SameElementVector<pm::Rational const&>, true> const&, pm::RepeatedRow<pm::SameElementVector<pm::Rational const&> > const&> const&, pm::Array<int, void> const&, pm::Array<int, void> const&> >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Wary< Matrix< Rational > > >, perl::Canned< const Matrix< Integer > >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Wary< pm::MatrixMinor<pm::Matrix<pm::Rational>&, pm::Array<int, void> const&, pm::all_selector const&> > >, perl::Canned< const pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Integer> const&>, pm::Series<int, true>, void> >);
   FunctionInstance4perl(new_X, Matrix< Integer >, perl::Canned< const pm::MatrixMinor<pm::Matrix<pm::Rational>&, pm::all_selector const&, pm::Complement<pm::SingleElementSet<int>, int, pm::operations::cmp> const&> >);
   OperatorInstance4perl(Binary_diva, perl::Canned< const Wary< pm::ColChain<pm::SingleCol<pm::Vector<pm::Rational> const&>, pm::Matrix<pm::Rational> const&> > >, perl::Canned< const pm::ColChain<pm::SingleCol<pm::Vector<pm::Rational> const&>, pm::Matrix<pm::Rational> const&> >);
   OperatorInstance4perl(Binary_diva, perl::Canned< const Wary< pm::RowChain<pm::ColChain<pm::SingleCol<pm::Vector<pm::Rational> const&>, pm::Matrix<pm::Rational> const&> const&, pm::ColChain<pm::SingleCol<pm::Vector<pm::Rational> const&>, pm::Matrix<pm::Rational> const&> const&> > >, perl::Canned< const pm::ColChain<pm::SingleCol<pm::Vector<pm::Rational> const&>, pm::Matrix<pm::Rational> const&> >);
   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const pm::RowChain<pm::RowChain<pm::ColChain<pm::SingleCol<pm::Vector<pm::Rational> const&>, pm::Matrix<pm::Rational> const&> const&, pm::ColChain<pm::SingleCol<pm::Vector<pm::Rational> const&>, pm::Matrix<pm::Rational> const&> const&> const&, pm::ColChain<pm::SingleCol<pm::Vector<pm::Rational> const&>, pm::Matrix<pm::Rational> const&> const&> >);
   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const pm::RowChain<pm::ColChain<pm::SingleCol<pm::Vector<pm::Rational> const&>, pm::Matrix<pm::Rational> const&> const&, pm::ColChain<pm::SingleCol<pm::Vector<pm::Rational> const&>, pm::Matrix<pm::Rational> const&> const&> >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Wary< Matrix< Rational > > >, perl::Canned< const pm::IndexedSlice<pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational>&>, pm::Series<int, true>, void>, pm::Complement<pm::SingleElementSet<int>, int, pm::operations::cmp> const&, void> >);
   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const pm::MatrixMinor<pm::Matrix<pm::Integer> const&, pm::all_selector const&, pm::Complement<pm::SingleElementSet<int>, int, pm::operations::cmp> const&> >);
   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const pm::ColChain<pm::SingleCol<pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational> const&>, pm::Series<int, true>, void> const&>, pm::Matrix<pm::Rational> const&> >);
   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const pm::RowChain<pm::SingleRow<pm::VectorChain<pm::SameElementVector<pm::Rational const&> const&, pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational>&>, pm::Series<int, true>, void> const&> const&>, pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::Rational const&> const&>, pm::Matrix<pm::Rational> const&> const&> >);
   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const pm::RowChain<pm::SingleRow<pm::VectorChain<pm::SameElementVector<pm::Rational const&> const&, pm::Vector<pm::Rational> const&> const&>, pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::Rational const&> const&>, pm::Matrix<pm::Rational> const&> const&> >);
   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const pm::RowChain<pm::SingleRow<pm::VectorChain<pm::SameElementVector<pm::Rational const&> const&, pm::IndexedSlice<pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational> const&>, pm::Series<int, true>, void> const&, pm::Series<int, true>, void> const&> const&>, pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::Rational const&> const&>, pm::Matrix<pm::Rational> const&> const&> >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
