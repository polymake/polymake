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

#include "polymake/client.h"
#include "polymake/SparseMatrix.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Vector.h"
#include "polymake/Rational.h"
#include "polymake/SparseVector.h"
#include "polymake/Integer.h"
#include "polymake/linalg.h"
#include "polymake/Matrix.h"
#include "polymake/QuadraticExtension.h"

namespace polymake { namespace common { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0>
   FunctionInterface4perl( new_int, T0 ) {
      perl::Value arg0(stack[1]);
      WrapperReturnNew(T0, (arg0.get<int>()) );
   };

   template <typename T0>
   FunctionInterface4perl( new, T0 ) {
      WrapperReturnNew(T0, () );
   };

   template <typename T0, typename T1>
   FunctionInterface4perl( new_X, T0,T1 ) {
      perl::Value arg0(stack[1]);
      WrapperReturnNew(T0, (arg0.get<T1>()) );
   };

   ClassTemplate4perl("Polymake::common::SparseVector");
   Class4perl("Polymake::common::SparseVector__Int", SparseVector< int >);
   Class4perl("Polymake::common::SparseVector__Float", SparseVector< double >);
   Class4perl("Polymake::common::SparseVector__Rational", SparseVector< Rational >);
   FunctionInstance4perl(new, SparseVector< Rational >);
   FunctionInstance4perl(new_X, SparseVector< double >, perl::Canned< const SparseVector< Rational > >);
   FunctionInstance4perl(new, SparseVector< double >);
   OperatorInstance4perl(convert, SparseVector< double >, perl::Canned< const Vector< Rational > >);
   OperatorInstance4perl(convert, SparseVector< double >, perl::Canned< const Vector< double > >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Wary< pm::sparse_matrix_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::Rational, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&, pm::NonSymmetric> > >, perl::Canned< const Vector< Rational > >);
   FunctionInstance4perl(new_X, SparseVector< Rational >, perl::Canned< const pm::sparse_matrix_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::Rational, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&, pm::NonSymmetric> >);
   FunctionInstance4perl(new_int, SparseVector< int >);
   Class4perl("Polymake::common::SparseVector__Integer", SparseVector< Integer >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Wary< pm::sparse_matrix_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::Rational, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&, pm::NonSymmetric> > >, perl::Canned< const pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Integer> const&>, pm::Series<int, true>, void> >);
   FunctionInstance4perl(new_X, SparseVector< double >, perl::Canned< const pm::SameElementSparseVector<pm::SingleElementSet<int>, double> >);
   OperatorInstance4perl(Unary_neg, perl::Canned< const Wary< pm::SameElementSparseVector<pm::SingleElementSet<int>, pm::Rational> > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Wary< SparseVector< Rational > > >, perl::Canned< const Vector< Rational > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Wary< SparseVector< Rational > > >, perl::Canned< const SparseVector< Rational > >);
   OperatorInstance4perl(Binary__ne, perl::Canned< const Wary< SparseVector< Rational > > >, perl::Canned< const SparseVector< Rational > >);
   OperatorInstance4perl(Binary__ne, perl::Canned< const Wary< SparseVector< Rational > > >, perl::Canned< const Vector< Rational > >);
   FunctionInstance4perl(new_X, SparseVector< Rational >, perl::Canned< const SparseVector< double > >);
   FunctionInstance4perl(new_X, SparseVector< QuadraticExtension< Rational > >, perl::Canned< const pm::SameElementSparseVector<pm::SingleElementSet<int>, pm::QuadraticExtension<pm::Rational> > >);
   FunctionInstance4perl(new_X, SparseVector< QuadraticExtension< Rational > >, perl::Canned< const pm::sparse_matrix_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::QuadraticExtension<pm::Rational>, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&, pm::NonSymmetric> >);
   Class4perl("Polymake::common::SparseVector__QuadraticExtension__Rational", SparseVector< QuadraticExtension< Rational > >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Wary< pm::IndexedSlice<pm::sparse_matrix_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::Rational, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&, pm::NonSymmetric> const&, pm::Series<int, true>, void> > >, perl::Canned< const pm::IndexedSlice<pm::sparse_matrix_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::Rational, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&, pm::NonSymmetric> const&, pm::Series<int, true>, void> >);
   OperatorInstance4perl(Binary_mul, double, perl::Canned< const Wary< SparseVector< double > > >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Wary< SparseVector< double > > >, perl::Canned< const SparseVector< double > >);
   FunctionInstance4perl(new, SparseVector< QuadraticExtension< Rational > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Wary< SparseVector< QuadraticExtension< Rational > > > >, perl::Canned< const Vector< QuadraticExtension< Rational > > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Wary< pm::sparse_matrix_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::Rational, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&, pm::NonSymmetric> > >, perl::Canned< const pm::VectorChain<pm::SingleElementVector<pm::Rational>, pm::Vector<pm::Rational> const&> >);
   OperatorInstance4perl(BinaryAssign__or, perl::Canned< SparseVector< int > >, perl::Canned< const Vector< int > >);
   OperatorInstance4perl(Unary_neg, perl::Canned< const Wary< pm::sparse_matrix_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<int, false, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> >&, pm::NonSymmetric> > >);
   OperatorInstance4perl(assign, pm::sparse_matrix_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<int, false, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> >&, pm::NonSymmetric>, perl::Canned< const SparseVector< int > >);
   OperatorInstance4perl(Binary_sub, perl::Canned< const Wary< pm::VectorChain<pm::sparse_matrix_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<int, false, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> >&, pm::NonSymmetric> const&, pm::Vector<int> const&> > >, perl::Canned< const Vector< int > >);
   FunctionInstance4perl(new_X, SparseVector< Integer >, perl::Canned< const pm::SameElementSparseVector<pm::SingleElementSet<int>, pm::Integer> >);
   FunctionInstance4perl(new, SparseVector< Integer >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Wary< SparseVector< Integer > > >, perl::Canned< const pm::SameElementSparseVector<pm::SingleElementSet<int>, pm::Integer> >);
   FunctionInstance4perl(new_int, SparseVector< Rational >);
   FunctionInstance4perl(new_int, SparseVector< double >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Wary< SparseVector< Rational > > >, perl::Canned< const Vector< Rational > >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Wary< SparseVector< Rational > > >, perl::Canned< const SparseVector< Rational > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Wary< pm::SameElementSparseVector<pm::SingleElementSet<int>, pm::Integer> > >, perl::Canned< const pm::SameElementSparseVector<pm::SingleElementSet<int>, pm::Integer> >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Wary< SparseVector< Integer > > >, perl::Canned< const SparseVector< Integer > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Wary< pm::sparse_matrix_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<int, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> >&, pm::NonSymmetric> > >, perl::Canned< const Vector< Rational > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Wary< SparseVector< QuadraticExtension< Rational > > > >, perl::Canned< const SparseVector< QuadraticExtension< Rational > > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
