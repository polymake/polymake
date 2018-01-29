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
#include "polymake/QuadraticExtension.h"
#include "polymake/Rational.h"
#include "polymake/Set.h"
#include "polymake/SparseMatrix.h"
#include "polymake/SparseVector.h"
#include "polymake/TropicalNumber.h"
#include "polymake/Vector.h"
#include "polymake/client.h"
#include "polymake/linalg.h"

namespace polymake { namespace common { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1>
   FunctionInterface4perl( new_X, T0,T1 ) {
      perl::Value arg0(stack[1]);
      WrapperReturnNew(T0, (arg0.get<T1>()) );
   };

   FunctionInstance4perl(new_X, Vector< QuadraticExtension< Rational > >, perl::Canned< const pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::QuadraticExtension<pm::Rational> > const&>, pm::Series<int, true>> >);
   OperatorInstance4perl(Binary_add, perl::Canned< const Wary< pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<double>&>, pm::Series<int, true>> > >, perl::Canned< const Vector< double > >);
   OperatorInstance4perl(Binary_add, perl::Canned< const Wary< Vector< double > > >, perl::Canned< const Vector< double > >);
   OperatorInstance4perl(Binary_sub, perl::Canned< const Wary< Vector< double > > >, perl::Canned< const Vector< double > >);
   OperatorInstance4perl(Binary_mul, int, perl::Canned< const Wary< Vector< double > > >);
   Class4perl("Polymake::common::Vector__Matrix_A_Rational_I_NonSymmetric_Z", Vector< Matrix< Rational > >);
   FunctionInstance4perl(new_X, Vector< int >, perl::Canned< const Array< int > >);
   OperatorInstance4perl(Binary_mul, int, perl::Canned< const Wary< Vector< int > > >);
   OperatorInstance4perl(Binary_mul, int, perl::Canned< const Wary< Vector< Integer > > >);
   FunctionInstance4perl(new_X, Vector< Integer >, perl::Canned< const pm::SameElementVector<pm::Integer const&> >);
   FunctionInstance4perl(new_X, Vector< Rational >, perl::Canned< const Vector< TropicalNumber< Max, Rational > > >);
   FunctionInstance4perl(new_X, Vector< Rational >, perl::Canned< const Vector< TropicalNumber< Min, Rational > > >);
   FunctionInstance4perl(new_X, Vector< Rational >, perl::Canned< const Array< int > >);
   FunctionInstance4perl(new_X, Vector< int >, perl::Canned< const pm::Series<int, true> >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Wary< Vector< Rational > > >, perl::Canned< const pm::SameElementSparseVector<pm::SingleElementSet<int>, pm::Rational> >);
   OperatorInstance4perl(assign, Vector< double >, perl::Canned< const Vector< Rational > >);
   OperatorInstance4perl(assign, pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<double>&>, pm::Series<int, true>, polymake::mlist<> >, perl::Canned< const pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational> const&>, pm::Series<int, true>, polymake::mlist<> > >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Wary< pm::IndexedSlice<pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<double> const&>, pm::Series<int, true>, mlist<> > const&, pm::Series<int, true>, mlist<> > > >, perl::Canned< const pm::IndexedSlice<pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<double> const&>, pm::Series<int, true>, mlist<> > const&, pm::Series<int, true>, mlist<> > >);
   OperatorInstance4perl(Binary_mul, double, perl::Canned< const Wary< pm::IndexedSlice<pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<double> const&>, pm::Series<int, true>, mlist<> > const&, pm::Series<int, true>, mlist<> > > >);
   OperatorInstance4perl(Unary_neg, perl::Canned< const Wary< pm::IndexedSlice<pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::QuadraticExtension<pm::Rational> > const&>, pm::Series<int, true>, mlist<> > const&, pm::Series<int, true>, mlist<> > > >);
   FunctionInstance4perl(new_X, Vector< double >, perl::Canned< const Vector< QuadraticExtension< Rational > > >);
   FunctionInstance4perl(new_X, Vector< Rational >, perl::Canned< const SparseVector< Rational > >);
   OperatorInstance4perl(Binary_mul, int, perl::Canned< const Wary< pm::IndexedSlice<pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational> const&>, pm::Series<int, true>, mlist<> > const&, pm::Series<int, true>, mlist<> > > >);
   OperatorInstance4perl(BinaryAssign_mul, perl::Canned< Wary< pm::IndexedSlice<pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational>&>, pm::Series<int, true>, mlist<> >&, pm::Complement<pm::SingleElementSetCmp<int, pm::operations::cmp>, int, pm::operations::cmp> const&, mlist<> > > >, int);
   OperatorInstance4perl(BinaryAssign_sub, perl::Canned< Wary< Vector< Rational > > >, perl::Canned< const Vector< Rational > >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Wary< pm::SameElementVector<pm::Rational const&> > >, perl::Canned< const pm::MatrixMinor<pm::Matrix<pm::Rational> const&, pm::Set<int, pm::operations::cmp> const&, pm::Complement<pm::SingleElementSetCmp<int, pm::operations::cmp>, int, pm::operations::cmp> const&> >);
   OperatorInstance4perl(BinaryAssign_sub, perl::Canned< Wary< Vector< Rational > > >, perl::Canned< const pm::SameElementVector<pm::Rational const&> >);
   OperatorInstance4perl(Binary__ora, perl::Canned< const Vector< Rational > >, perl::Canned< const pm::MatrixMinor<pm::Matrix<pm::Rational> const&, pm::all_selector const&, pm::Complement<pm::SingleElementSetCmp<int, pm::operations::cmp>, int, pm::operations::cmp> const&> >);
   OperatorInstance4perl(assign, Vector< Integer >, perl::Canned< const Vector< int > >);
   OperatorInstance4perl(assign, Vector< Integer >, perl::Canned< const SparseVector< int > >);
   FunctionInstance4perl(new_X, Vector< int >, perl::Canned< const Vector< Rational > >);
   OperatorInstance4perl(assign, pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational>&>, pm::Series<int, true>, mlist<> >, perl::Canned< const Vector< double > >);
   OperatorInstance4perl(assign, pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational>&>, pm::Series<int, true>, mlist<> >, perl::Canned< const Vector< QuadraticExtension< Rational > > >);
   OperatorInstance4perl(Binary_mul, int, perl::Canned< const Wary< pm::IndexedSlice<pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::QuadraticExtension<pm::Rational> > const&>, pm::Series<int, true>, mlist<> > const&, pm::Series<int, true>, mlist<> > > >);
   FunctionInstance4perl(new_X, Vector< Integer >, perl::Canned< const pm::SameElementSparseVector<pm::SingleElementSetCmp<int, pm::operations::cmp>, pm::Integer> >);
   OperatorInstance4perl(Binary__ora, perl::Canned< const pm::SameElementVector<double const&> >, perl::Canned< const pm::RowChain<pm::Matrix<double> const&, pm::SingleRow<pm::Vector<double> const&> > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
