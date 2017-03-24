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

#include "polymake/Array.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Integer.h"
#include "polymake/Matrix.h"
#include "polymake/QuadraticExtension.h"
#include "polymake/Rational.h"
#include "polymake/SparseMatrix.h"
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

   template <typename T0>
   FunctionInterface4perl( new_int_int, T0 ) {
      perl::Value arg0(stack[1]), arg1(stack[2]);
      WrapperReturnNew(T0, (arg0.get<int>(), arg1.get<int>()) );
   };

   FunctionInstance4perl(new_int_int, SparseMatrix< double, NonSymmetric >);
   OperatorInstance4perl(Binary_diva, perl::Canned< const Wary< pm::DiagMatrix<pm::SameElementVector<pm::Rational const&>, true> > >, perl::Canned< const pm::RepeatedRow<pm::SameElementVector<pm::Rational const&> > >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Wary< SparseMatrix< Integer, NonSymmetric > > >, perl::Canned< const pm::ColChain<pm::DiagMatrix<pm::SameElementVector<pm::Rational const&>, true> const&, pm::RepeatedRow<pm::SameElementVector<pm::Rational const&> > const&> >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Wary< SparseMatrix< Integer, NonSymmetric > > >, perl::Canned< const pm::RowChain<pm::DiagMatrix<pm::SameElementVector<pm::Rational const&>, true> const&, pm::RepeatedRow<pm::SameElementVector<pm::Rational const&> > const&> >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Wary< pm::RowChain<pm::DiagMatrix<pm::SameElementVector<pm::Rational const&>, true> const&, pm::RepeatedRow<pm::SameElementVector<pm::Rational const&> > const&> > >, perl::Canned< const Matrix< Integer > >);
   OperatorInstance4perl(Binary_diva, perl::Canned< const Wary< pm::DiagMatrix<pm::SameElementVector<pm::Rational const&>, true> > >, perl::Canned< const Vector< Rational > >);
   OperatorInstance4perl(Unary_neg, perl::Canned< const Wary< pm::DiagMatrix<pm::SameElementVector<pm::Rational const&>, true> > >);
   OperatorInstance4perl(Binary_diva, perl::Canned< const Wary< pm::DiagMatrix<pm::SameElementVector<pm::Rational const&>, true> > >, perl::Canned< const SparseMatrix< Rational, Symmetric > >);
   OperatorInstance4perl(Unary_neg, perl::Canned< const Wary< SparseMatrix< Rational, NonSymmetric > > >);
   FunctionInstance4perl(new_int_int, SparseMatrix< QuadraticExtension< Rational >, NonSymmetric >);
   Class4perl("Polymake::common::SparseMatrix_A_Float_I_Symmetric_Z", SparseMatrix< double, Symmetric >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Wary< SparseMatrix< double, NonSymmetric > > >, perl::Canned< const Matrix< double > >);
   FunctionInstance4perl(new_X, SparseMatrix< Rational, NonSymmetric >, perl::Canned< const pm::MatrixMinor<pm::SparseMatrix<pm::QuadraticExtension<pm::Rational>, pm::NonSymmetric> const&, pm::Array<int> const&, pm::all_selector const&> >);
   FunctionInstance4perl(new_X, SparseMatrix< Rational, NonSymmetric >, perl::Canned< const pm::RowChain<pm::SparseMatrix<pm::Rational, pm::NonSymmetric> const&, pm::Matrix<pm::Rational> const&> >);
   FunctionInstance4perl(new_X, SparseMatrix< int, NonSymmetric >, perl::Canned< const pm::DiagMatrix<pm::SameElementVector<pm::Rational const&>, true> >);
   Class4perl("Polymake::common::SparseMatrix_A_TropicalNumber_A_Min_I_Rational_Z_I_Symmetric_Z", SparseMatrix< TropicalNumber< Min, Rational >, Symmetric >);
   OperatorInstance4perl(Binary__ora, perl::Canned< const Wary< SparseMatrix< int, NonSymmetric > > >, perl::Canned< const Matrix< int > >);
   OperatorInstance4perl(Unary_neg, perl::Canned< const Wary< pm::ColChain<pm::SingleCol<pm::Vector<pm::Rational> const&>, pm::SparseMatrix<pm::Rational, pm::NonSymmetric> const&> > >);
   FunctionInstance4perl(new_X, SparseMatrix< Rational, NonSymmetric >, perl::Canned< const pm::ColChain<pm::SingleCol<pm::Vector<pm::Rational> const&>, pm::SparseMatrix<pm::Rational, pm::NonSymmetric> const&> >);
   OperatorInstance4perl(Binary__ora, perl::Canned< const Wary< SparseMatrix< Rational, NonSymmetric > > >, perl::Canned< const Vector< Rational > >);
   FunctionInstance4perl(new_X, SparseMatrix< Rational, NonSymmetric >, perl::Canned< const pm::ColChain<pm::SparseMatrix<pm::Rational, pm::NonSymmetric> const&, pm::SingleCol<pm::Vector<pm::Rational> const&> > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
