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

///==== this line controls the automatic file splitting: max.instances=40

#include "polymake/client.h"
#include "polymake/Vector.h"
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Integer.h"
#include "polymake/SparseVector.h"
#include "polymake/linalg.h"
#include "polymake/SparseMatrix.h"
#include "polymake/Array.h"
#include "polymake/QuadraticExtension.h"
#include "polymake/Set.h"

namespace polymake { namespace common { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1>
   FunctionInterface4perl( new_X, T0,T1 ) {
      perl::Value arg0(stack[1]);
      WrapperReturnNew(T0, (arg0.get<T1>()) );
   };

   OperatorInstance4perl(assign, pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational>&>, pm::Series<int, true>>, perl::Canned< const pm::VectorChain<pm::SameElementSparseVector<pm::SingleElementSet<int>, pm::Rational> const&, pm::SameElementSparseVector<pm::SingleElementSet<int>, pm::Rational> const&> >);
   FunctionInstance4perl(new_X, Vector< Rational >, perl::Canned< const pm::VectorChain<pm::SameElementSparseVector<pm::SingleElementSet<int>, pm::Rational> const&, pm::SameElementSparseVector<pm::SingleElementSet<int>, pm::Rational> const&> >);
   OperatorInstance4perl(assign, pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational>&>, pm::Series<int, true>>, perl::Canned< const pm::sparse_matrix_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::Rational, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&, pm::NonSymmetric> >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Wary< Vector< Integer > > >, perl::Canned< const Vector< Integer > >);
   OperatorInstance4perl(Binary_sub, perl::Canned< const Wary< pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational>&>, pm::Series<int, true>> > >, perl::Canned< const pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational>&>, pm::Series<int, true>> >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Wary< Vector< Rational > > >, perl::Canned< const Vector< Rational > >);
   FunctionInstance4perl(new_X, Vector< Rational >, perl::Canned< const Array< Rational > >);
   FunctionInstance4perl(new_X, Vector< Rational >, perl::Canned< const Vector< int > >);
   FunctionInstance4perl(new_X, Vector< Integer >, perl::Canned< const Array< Integer > >);
   FunctionInstance4perl(new_X, Vector< int >, perl::Canned< const pm::SameElementVector<pm::Rational const&> >);
   FunctionInstance4perl(new_X, Vector< int >, perl::Canned< const pm::SameElementVector<int const&> >);
   FunctionInstance4perl(new_X, Vector< int >, perl::Canned< const pm::SameElementSparseVector<pm::SingleElementSet<int>, int> >);
   OperatorInstance4perl(assign, pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<int>&>, pm::Series<int, true>>, perl::Canned< const pm::VectorChain<pm::VectorChain<pm::sparse_matrix_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<int, false, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> >&, pm::NonSymmetric> const&, pm::Vector<int> const&> const&, pm::Vector<int> const&> >);
   OperatorInstance4perl(Binary_mul, int, perl::Canned< const Wary< pm::SameElementVector<int const&> > >);
   FunctionInstance4perl(new_X, Vector< int >, perl::Canned< const pm::VectorChain<pm::SameElementVector<int const&> const&, pm::SameElementVector<int const&> const&> >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Wary< pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational>&>, pm::Series<int, true>> > >, perl::Canned< const pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<int> const&>, pm::Series<int, true>> >);
   OperatorInstance4perl(Binary__ora, perl::Canned< const pm::SameElementVector<pm::Integer const&> >, perl::Canned< const Matrix< Integer > >);
   OperatorInstance4perl(Binary__ora, perl::Canned< const pm::SameElementVector<pm::Rational const&> >, perl::Canned< const Matrix< Rational > >);
   OperatorInstance4perl(Binary__ora, perl::Canned< const pm::SameElementVector<pm::Rational const&> >, perl::Canned< const pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::Rational const&> const&>, pm::Matrix<pm::Rational> const&> >);
   OperatorInstance4perl(Binary__ora, perl::Canned< const pm::SameElementVector<pm::Rational const&> >, perl::Canned< const pm::RepeatedRow<pm::SameElementVector<pm::Rational const&> > >);
   OperatorInstance4perl(Binary_div, perl::Canned< const Wary< Vector< double > > >, perl::Canned< const Matrix< double > >);
   OperatorInstance4perl(Binary__ora, perl::Canned< const pm::SameElementVector<pm::QuadraticExtension<pm::Rational> const&> >, perl::Canned< const Matrix< QuadraticExtension< Rational > > >);
   OperatorInstance4perl(Binary__ora, perl::Canned< const pm::SameElementVector<double const&> >, perl::Canned< const pm::RowChain<pm::MatrixMinor<pm::Matrix<double>&, pm::incidence_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::nothing, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&> const&, pm::all_selector const&> const&, pm::SingleRow<pm::Vector<double> const&> > >);
   OperatorInstance4perl(Binary__ora, perl::Canned< const pm::SameElementVector<double const&> >, perl::Canned< const Matrix< double > >);
   OperatorInstance4perl(Binary__ora, perl::Canned< const pm::SameElementVector<double const&> >, perl::Canned< const pm::ColChain<pm::SingleCol<pm::SameElementVector<double const&> const&>, pm::Matrix<double> const&> >);
   OperatorInstance4perl(Binary__ora, perl::Canned< const Vector< int > >, perl::Canned< const Matrix< int > >);
   OperatorInstance4perl(Binary__ora, perl::Canned< const Vector< Rational > >, perl::Canned< const pm::MatrixMinor<pm::Matrix<pm::Rational> const&, pm::incidence_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::nothing, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&> const&, pm::Series<int, true> const&> >);
   OperatorInstance4perl(Binary_div, perl::Canned< const Wary< pm::SameElementVector<int const&> > >, perl::Canned< const pm::DiagMatrix<pm::SameElementVector<int const&>, true> >);
   OperatorInstance4perl(Binary__ora, perl::Canned< const Vector< int > >, perl::Canned< const pm::MatrixMinor<pm::Matrix<int> const&, pm::Complement<pm::Set<int, pm::operations::cmp>, int, pm::operations::cmp> const&, pm::all_selector const&> >);
   OperatorInstance4perl(Binary__ora, perl::Canned< const pm::SameElementVector<pm::Rational const&> >, perl::Canned< const SparseMatrix< Rational, NonSymmetric > >);
   OperatorInstance4perl(Binary__ora, perl::Canned< const pm::SameElementVector<pm::Rational const&> >, perl::Canned< const pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::Rational const&> const&>, pm::SparseMatrix<pm::Rational, pm::NonSymmetric> const&> >);
   OperatorInstance4perl(convert, Vector< Integer >, perl::Canned< const SparseVector< Integer > >);
   OperatorInstance4perl(Binary__ora, perl::Canned< const Vector< Integer > >, perl::Canned< const pm::SameElementVector<pm::Integer const&> >);
   FunctionInstance4perl(new_X, Vector< Integer >, perl::Canned< const pm::VectorChain<pm::Vector<pm::Integer> const&, pm::SameElementVector<pm::Integer const&> const&> >);
   OperatorInstance4perl(convert, Vector< double >, perl::Canned< const Vector< QuadraticExtension< Rational > > >);
   OperatorInstance4perl(Binary__ora, perl::Canned< const pm::SameElementVector<pm::Rational const&> >, perl::Canned< const pm::MatrixMinor<pm::Matrix<pm::Rational> const&, pm::Array<int> const&, pm::all_selector const&> >);
   OperatorInstance4perl(Binary_div, perl::Canned< const Wary< pm::SameElementVector<int const&> > >, perl::Canned< const SparseMatrix< int, NonSymmetric > >);
   OperatorInstance4perl(assign, pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational>&>, pm::Series<int, true>>, perl::Canned< const SparseVector< Rational > >);
   OperatorInstance4perl(Binary__ora, perl::Canned< const Vector< Rational > >, perl::Canned< const Matrix< Rational > >);
   OperatorInstance4perl(assign, pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational>&>, pm::Series<int, true>>, perl::Canned< const pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Integer> const&>, pm::Series<int, true>> >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
