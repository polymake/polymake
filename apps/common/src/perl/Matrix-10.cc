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
#include "polymake/Rational.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/SparseMatrix.h"
#include "polymake/Set.h"
#include "polymake/QuadraticExtension.h"
#include "polymake/Vector.h"
#include "polymake/linalg.h"
#include "polymake/Integer.h"

namespace polymake { namespace common { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1>
   FunctionInterface4perl( new_X, T0,T1 ) {
      perl::Value arg0(stack[1]);
      WrapperReturnNew(T0, (arg0.get<T1>()) );
   };

   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const pm::RowChain<pm::SparseMatrix<pm::Rational, pm::NonSymmetric> const&, pm::MatrixMinor<pm::Matrix<pm::Rational> const&, pm::Set<int, pm::operations::cmp> const&, pm::Series<int, true> const&> const&> >);
   FunctionInstance4perl(new_X, Matrix< QuadraticExtension< Rational > >, perl::Canned< const pm::Transposed<pm::Matrix<pm::QuadraticExtension<pm::Rational> > > >);
   FunctionInstance4perl(new_X, Matrix< double >, perl::Canned< const Matrix< QuadraticExtension< Rational > > >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Wary< Matrix< QuadraticExtension< Rational > > > >, perl::Canned< const Vector< QuadraticExtension< Rational > > >);
   OperatorInstance4perl(assign, Matrix< QuadraticExtension< Rational > >, perl::Canned< const SparseMatrix< QuadraticExtension< Rational >, NonSymmetric > >);
   FunctionInstance4perl(new_X, Matrix< QuadraticExtension< Rational > >, perl::Canned< const SparseMatrix< QuadraticExtension< Rational >, NonSymmetric > >);
   OperatorInstance4perl(Binary_sub, perl::Canned< const Wary< pm::MatrixMinor<pm::Matrix<pm::Rational> const&, pm::incidence_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::nothing, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&> const&, pm::all_selector const&> > >, perl::Canned< const pm::RepeatedRow<pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Integer> const&>, pm::Series<int, true>> const&> >);
   OperatorInstance4perl(Unary_neg, perl::Canned< const Wary< pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::QuadraticExtension<pm::Rational> const&> const&>, pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::QuadraticExtension<pm::Rational> const&> const&>, pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&> const&> > >);
   FunctionInstance4perl(new_X, Matrix< QuadraticExtension< Rational > >, perl::Canned< const pm::RowChain<pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&, pm::SingleRow<pm::Vector<pm::QuadraticExtension<pm::Rational> > const&> > >);
   OperatorInstance4perl(BinaryAssign_div, perl::Canned< Wary< Matrix< double > > >, perl::Canned< const SparseMatrix< double, NonSymmetric > >);
   OperatorInstance4perl(assign, Matrix< Rational >, perl::Canned< const Matrix< Integer > >);
   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const Set< Vector< Rational > > >);
   OperatorInstance4perl(assign, Matrix< double >, perl::Canned< const SparseMatrix< double, NonSymmetric > >);
   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const pm::ColChain<pm::Transposed<pm::Matrix<pm::Rational> > const&, pm::SingleCol<pm::SameElementSparseVector<pm::SingleElementSet<int>, pm::Rational> const&> > >);
   OperatorInstance4perl(assign, Matrix< Integer >, perl::Canned< const Matrix< int > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
