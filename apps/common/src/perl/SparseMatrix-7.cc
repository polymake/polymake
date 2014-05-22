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

///==== this line controls the automatic file splitting: max.instances=20

#include "polymake/client.h"
#include "polymake/SparseMatrix.h"
#include "polymake/linalg.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Set.h"
#include "polymake/Matrix.h"
#include "polymake/QuadraticExtension.h"
#include "polymake/Rational.h"
#include "polymake/Vector.h"
#include "polymake/Integer.h"
#include "polymake/RationalFunction.h"

namespace polymake { namespace common { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1>
   FunctionInterface4perl( new_X, T0,T1 ) {
      perl::Value arg0(stack[1]);
      WrapperReturnNew(T0, (arg0.get<T1>()) );
   };

   FunctionInstance4perl(new_X, SparseMatrix< Rational, NonSymmetric >, perl::Canned< const SparseMatrix< QuadraticExtension< Rational >, NonSymmetric > >);
   Class4perl("Polymake::common::SparseMatrix_A_QuadraticExtension__Rational_I_Symmetric_Z", SparseMatrix< QuadraticExtension< Rational >, Symmetric >);
   FunctionInstance4perl(new_X, SparseMatrix< QuadraticExtension< Rational >, NonSymmetric >, perl::Canned< const pm::DiagMatrix<pm::SameElementVector<pm::QuadraticExtension<pm::Rational> const&>, true> >);
   FunctionInstance4perl(new_X, SparseMatrix< Rational, NonSymmetric >, perl::Canned< const pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::Integer const&> const&>, pm::SparseMatrix<pm::Integer, pm::NonSymmetric> const&> >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Wary< SparseMatrix< int, NonSymmetric > > >, perl::Canned< const pm::Transposed<pm::SparseMatrix<int, pm::NonSymmetric> > >);
   FunctionInstance4perl(new_X, SparseMatrix< Rational, NonSymmetric >, perl::Canned< const pm::ColChain<pm::Matrix<pm::Rational> const&, pm::DiagMatrix<pm::SameElementVector<pm::Rational const&>, true> const&> >);
   FunctionInstance4perl(new_X, SparseMatrix< Rational, NonSymmetric >, perl::Canned< const pm::RowChain<pm::ColChain<pm::Matrix<pm::Rational> const&, pm::DiagMatrix<pm::SameElementVector<pm::Rational const&>, true> const&> const&, pm::ColChain<pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::Rational const&> const&>, pm::RepeatedRow<pm::SameElementVector<pm::Rational const&> > const&> const&, pm::DiagMatrix<pm::SameElementVector<pm::Rational const&>, true> const&> const&> >);
   FunctionInstance4perl(new_X, SparseMatrix< Rational, NonSymmetric >, perl::Canned< const SparseMatrix< int, NonSymmetric > >);
   Class4perl("Polymake::common::SparseMatrix_A_UniPolynomial_A_Rational_I_Int_Z_I_Symmetric_Z", SparseMatrix< UniPolynomial< Rational, int >, Symmetric >);
   OperatorInstance4perl(BinaryAssign__or, perl::Canned< Wary< SparseMatrix< int, NonSymmetric > > >, perl::Canned< const Matrix< int > >);
   OperatorInstance4perl(BinaryAssign_div, perl::Canned< Wary< SparseMatrix< int, NonSymmetric > > >, perl::Canned< const pm::ColChain<pm::Matrix<int> const&, pm::Matrix<int> const&> >);
   OperatorInstance4perl(Binary__ne, perl::Canned< const Wary< SparseMatrix< int, NonSymmetric > > >, perl::Canned< const SparseMatrix< int, NonSymmetric > >);
   OperatorInstance4perl(Binary_diva, perl::Canned< const Wary< SparseMatrix< Rational, NonSymmetric > > >, perl::Canned< const Matrix< Rational > >);
   OperatorInstance4perl(Binary_diva, perl::Canned< const Wary< pm::ColChain<pm::Matrix<pm::Rational> const&, pm::DiagMatrix<pm::SameElementVector<pm::Rational const&>, true> const&> > >, perl::Canned< const pm::ColChain<pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::Rational const&> const&>, pm::RepeatedRow<pm::SameElementVector<pm::Rational const&> > const&> const&, pm::DiagMatrix<pm::SameElementVector<pm::Rational const&>, true> const&> >);
   FunctionInstance4perl(new_X, SparseMatrix< int, NonSymmetric >, perl::Canned< const pm::MatrixMinor<pm::RowChain<pm::SingleRow<pm::SameElementVector<int const&> const&>, pm::DiagMatrix<pm::SameElementVector<int const&>, true> const&> const&, pm::Complement<pm::SingleElementSet<int>, int, pm::operations::cmp> const&, pm::all_selector const&> >);
   OperatorInstance4perl(Binary_diva, perl::Canned< const Wary< SparseMatrix< Rational, NonSymmetric > > >, perl::Canned< const SparseMatrix< Rational, NonSymmetric > >);
   OperatorInstance4perl(Binary_diva, perl::Canned< const Wary< SparseMatrix< QuadraticExtension< Rational >, NonSymmetric > > >, perl::Canned< const SparseMatrix< QuadraticExtension< Rational >, NonSymmetric > >);
   OperatorInstance4perl(Binary_diva, perl::Canned< const Wary< SparseMatrix< Rational, NonSymmetric > > >, perl::Canned< const Vector< Rational > >);
   OperatorInstance4perl(Binary_diva, perl::Canned< const Wary< SparseMatrix< QuadraticExtension< Rational >, NonSymmetric > > >, perl::Canned< const Matrix< QuadraticExtension< Rational > > >);
   OperatorInstance4perl(Binary_mul, int, perl::Canned< const Wary< pm::DiagMatrix<pm::SameElementVector<int const&> const&, false> > >);
   FunctionInstance4perl(new_X, SparseMatrix< int, NonSymmetric >, perl::Canned< const pm::MatrixMinor<pm::RowChain<pm::SingleRow<pm::SameElementVector<int const&> const&>, pm::SparseMatrix<int, pm::NonSymmetric> const&> const&, pm::Complement<pm::SingleElementSet<int>, int, pm::operations::cmp> const&, pm::all_selector const&> >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
