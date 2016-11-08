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

#include "polymake/IncidenceMatrix.h"
#include "polymake/Integer.h"
#include "polymake/Matrix.h"
#include "polymake/QuadraticExtension.h"
#include "polymake/Rational.h"
#include "polymake/Set.h"
#include "polymake/SparseMatrix.h"
#include "polymake/SparseVector.h"
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

   OperatorInstance4perl(Binary_add, perl::Canned< const Wary< pm::SameElementVector<pm::Rational const&> > >, perl::Canned< const pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Integer> const&>, pm::Series<int, true>> >);
   OperatorInstance4perl(assign, pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Integer>&>, pm::Series<int, true>>, perl::Canned< const Vector< Rational > >);
   OperatorInstance4perl(Binary__ne, perl::Canned< const Wary< pm::IndexedSlice<pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational>&>, pm::Series<int, true>>, pm::Complement<pm::SingleElementSet<int>, int, pm::operations::cmp> const&> > >, perl::Canned< const Vector< Rational > >);
   OperatorInstance4perl(Binary_sub, perl::Canned< const Wary< pm::IndexedSlice<pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational>&>, pm::Series<int, true>>, pm::Complement<pm::SingleElementSet<int>, int, pm::operations::cmp> const&> > >, perl::Canned< const Vector< Rational > >);
   OperatorInstance4perl(Binary__ora, perl::Canned< const pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational> const&>, pm::Series<int, true>> >, perl::Canned< const Matrix< Rational > >);
   OperatorInstance4perl(assign, Vector< Rational >, perl::Canned< const pm::IndexedSlice<pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational> const&>, pm::Series<int, true>> const&, pm::Series<int, true>> >);
   OperatorInstance4perl(assign, pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational>&>, pm::Series<int, true>>, perl::Canned< const pm::IndexedSlice<pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational>&>, pm::Series<int, true>>, pm::Complement<pm::SingleElementSet<int>, int, pm::operations::cmp> const&> >);
   OperatorInstance4perl(Binary__ora, perl::Canned< const pm::SameElementVector<pm::Rational const&> >, perl::Canned< const pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational>&>, pm::Series<int, true>> >);
   OperatorInstance4perl(Binary_div, perl::Canned< const Wary< pm::VectorChain<pm::SameElementVector<pm::Rational const&> const&, pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational>&>, pm::Series<int, true>> const&> > >, perl::Canned< const pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::Rational const&> const&>, pm::Matrix<pm::Rational> const&> >);
   OperatorInstance4perl(Binary_sub, perl::Canned< const Wary< pm::IndexedSlice<pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational> const&>, pm::Series<int, true>> const&, pm::Series<int, true>> > >, perl::Canned< const Vector< Rational > >);
   OperatorInstance4perl(Binary_sub, perl::Canned< const Wary< pm::IndexedSlice<pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational> const&>, pm::Series<int, true>> const&, pm::Series<int, true>> > >, perl::Canned< const pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational>&>, pm::Series<int, true>> >);
   OperatorInstance4perl(Binary__ora, perl::Canned< const pm::SameElementVector<pm::Rational const&> >, perl::Canned< const Vector< Rational > >);
   OperatorInstance4perl(Binary_div, perl::Canned< const Wary< pm::VectorChain<pm::SameElementVector<pm::Rational const&> const&, pm::Vector<pm::Rational> const&> > >, perl::Canned< const pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::Rational const&> const&>, pm::Matrix<pm::Rational> const&> >);
   OperatorInstance4perl(Binary__ora, perl::Canned< const pm::SameElementVector<pm::Rational const&> >, perl::Canned< const pm::IndexedSlice<pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational> const&>, pm::Series<int, true>> const&, pm::Series<int, true>> >);
   OperatorInstance4perl(Binary_div, perl::Canned< const Wary< pm::VectorChain<pm::SameElementVector<pm::Rational const&> const&, pm::IndexedSlice<pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational> const&>, pm::Series<int, true>> const&, pm::Series<int, true>> const&> > >, perl::Canned< const pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::Rational const&> const&>, pm::Matrix<pm::Rational> const&> >);
   OperatorInstance4perl(Binary_add, perl::Canned< const Wary< Vector< Rational > > >, perl::Canned< const pm::IndexedSlice<pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational> const&>, pm::Series<int, true>> const&, pm::Series<int, true>> >);
   FunctionInstance4perl(new_X, Vector< Rational >, perl::Canned< const pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Integer> const&>, pm::Series<int, true>> >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Wary< pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Integer> const&>, pm::Series<int, true>> > >, perl::Canned< const pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Integer> const&>, pm::Series<int, true>> >);
   OperatorInstance4perl(assign, pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational>&>, pm::Series<int, true>>, perl::Canned< const Vector< Integer > >);
   OperatorInstance4perl(Binary_add, perl::Canned< const Wary< pm::IndexedSlice<pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Integer> const&>, pm::Series<int, true>> const&, pm::Series<int, true>> > >, perl::Canned< const pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Integer> const&>, pm::Series<int, true>> >);
   OperatorInstance4perl(Unary_neg, perl::Canned< const Wary< pm::SameElementVector<pm::Rational const&> > >);
   OperatorInstance4perl(Binary__ora, perl::Canned< const pm::IndexedSlice<pm::Vector<pm::Rational> const&, pm::incidence_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::nothing, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&> const&> >, perl::Canned< const Matrix< Rational > >);
   OperatorInstance4perl(Binary_sub, perl::Canned< const Wary< Vector< Integer > > >, perl::Canned< const Vector< Rational > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Wary< Vector< Rational > > >, perl::Canned< const pm::SameElementVector<pm::Rational const&> >);
   OperatorInstance4perl(Binary_div, perl::Canned< const Wary< pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational>&>, pm::Series<int, true>> > >, perl::Canned< const Rational >);
   OperatorInstance4perl(Binary__ne, perl::Canned< const Wary< Vector< Rational > > >, perl::Canned< const Vector< Integer > >);
   OperatorInstance4perl(Binary_div, perl::Canned< const Wary< Vector< double > > >, double);
   OperatorInstance4perl(Binary__ora, perl::Canned< const pm::IndexedSlice<pm::Vector<pm::Rational> const&, pm::incidence_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::nothing, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&> const&> >, perl::Canned< const pm::MatrixMinor<pm::Matrix<pm::Rational> const&, pm::incidence_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::nothing, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&> const&, pm::all_selector const&> >);
   OperatorInstance4perl(Binary_sub, perl::Canned< const Wary< Vector< Rational > > >, perl::Canned< const Vector< Rational > >);
   OperatorInstance4perl(Unary_neg, perl::Canned< const Wary< Vector< double > > >);
   FunctionInstance4perl(new_X, Vector< Integer >, perl::Canned< const Vector< int > >);
   Class4perl("Polymake::common::Vector__Pair_A_Float_I_Float_Z", Vector< std::pair< double, double > >);
   OperatorInstance4perl(Binary__ora, perl::Canned< const pm::SameElementVector<pm::Rational const&> >, perl::Canned< const pm::MatrixMinor<pm::Matrix<pm::Rational> const&, pm::all_selector const&, pm::Complement<pm::SingleElementSet<int>, int, pm::operations::cmp> const&> >);
   OperatorInstance4perl(Binary_add, perl::Canned< const Wary< pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::QuadraticExtension<pm::Rational> > const&>, pm::Series<int, true>> > >, perl::Canned< const pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::QuadraticExtension<pm::Rational> > const&>, pm::Series<int, true>> >);
   FunctionInstance4perl(new_X, Vector< Integer >, perl::Canned< const SparseVector< Integer > >);
   OperatorInstance4perl(Binary__ora, int, perl::Canned< const Vector< double > >);
   OperatorInstance4perl(Binary_div, perl::Canned< const Wary< pm::VectorChain<pm::SingleElementVector<double>, pm::Vector<double> const&> > >, perl::Canned< const Matrix< double > >);
   OperatorInstance4perl(assign, pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<double>&>, pm::Series<int, true>>, perl::Canned< const pm::SameElementSparseVector<pm::SingleElementSet<int>, double> >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Wary< Vector< Rational > > >, perl::Canned< const pm::sparse_matrix_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<int, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&, pm::NonSymmetric> >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
