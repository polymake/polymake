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

#include "polymake/IncidenceMatrix.h"
#include "polymake/Integer.h"
#include "polymake/Matrix.h"
#include "polymake/QuadraticExtension.h"
#include "polymake/Rational.h"
#include "polymake/Set.h"
#include "polymake/SparseMatrix.h"
#include "polymake/TropicalNumber.h"
#include "polymake/Vector.h"
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

   OperatorInstance4perl(Binary_mul, perl::Canned< const Wary< Matrix< QuadraticExtension< Rational > > > >, perl::Canned< const pm::Transposed<pm::Matrix<pm::QuadraticExtension<pm::Rational> > > >);
   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const pm::ColChain<pm::SingleCol<pm::SameElementVector<double const&> const&>, pm::Matrix<double> const&> >);
   FunctionInstance4perl(new_X, Matrix< double >, perl::Canned< const pm::ColChain<pm::SingleCol<pm::SameElementVector<double const&> const&>, pm::Matrix<double> const&> >);
   OperatorInstance4perl(Binary_diva, perl::Canned< const Wary< pm::ColChain<pm::SingleCol<pm::SameElementVector<double const&> const&>, pm::Matrix<double> const&> > >, perl::Canned< const pm::ColChain<pm::SingleCol<pm::SameElementVector<double const&> const&>, pm::Matrix<double> const&> >);
   OperatorInstance4perl(Binary_diva, perl::Canned< const Wary< Matrix< double > > >, int);
   OperatorInstance4perl(Unary_neg, perl::Canned< const Wary< Matrix< QuadraticExtension< Rational > > > >);
   OperatorInstance4perl(assign, Matrix< Rational >, perl::Canned< const Matrix< int > >);
   OperatorInstance4perl(convert, Matrix< Rational >, perl::Canned< const pm::RowChain<pm::RowChain<pm::Matrix<pm::Integer> const&, pm::Matrix<pm::Integer> const&> const&, pm::Matrix<pm::Integer> const&> >);
   OperatorInstance4perl(convert, Matrix< Rational >, perl::Canned< const SparseMatrix< int, NonSymmetric > >);
   OperatorInstance4perl(Binary_diva, perl::Canned< const Wary< Matrix< QuadraticExtension< Rational > > > >, perl::Canned< const Vector< QuadraticExtension< Rational > > >);
   OperatorInstance4perl(convert, Matrix< QuadraticExtension< Rational > >, perl::Canned< const SparseMatrix< Rational, NonSymmetric > >);
   FunctionInstance4perl(new_X, Matrix< TropicalNumber< Min, Rational > >, perl::Canned< const pm::DiagMatrix<pm::SameElementVector<pm::TropicalNumber<pm::Min, pm::Rational> const&>, true> >);
   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::Rational const&> const&>, pm::RowChain<pm::DiagMatrix<pm::SameElementVector<pm::Rational const&>, true> const&, pm::DiagMatrix<pm::SameElementVector<pm::Rational const&>, true> const&> const&> >);
   Class4perl("Polymake::common::Matrix_A_TropicalNumber_A_Min_I_Rational_Z_I_NonSymmetric_Z", Matrix< TropicalNumber< Min, Rational > >);
   FunctionInstance4perl(new_int_int, Matrix< TropicalNumber< Min, Rational > >);
   OperatorInstance4perl(Binary_add, perl::Canned< const Wary< Matrix< TropicalNumber< Min, Rational > > > >, perl::Canned< const Matrix< TropicalNumber< Min, Rational > > >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Wary< Matrix< TropicalNumber< Min, Rational > > > >, perl::Canned< const Matrix< TropicalNumber< Min, Rational > > >);
   OperatorInstance4perl(BinaryAssign_div, perl::Canned< Wary< Matrix< QuadraticExtension< Rational > > > >, perl::Canned< const Matrix< QuadraticExtension< Rational > > >);
   OperatorInstance4perl(Binary__ora, perl::Canned< const Wary< pm::MatrixMinor<pm::Matrix<pm::Rational>&, pm::all_selector const&, pm::Complement<pm::SingleElementSet<int>, int, pm::operations::cmp> const&> > >, perl::Canned< const Vector< Rational > >);
   FunctionInstance4perl(new_X, Matrix< int >, perl::Canned< const pm::ColChain<pm::MatrixMinor<pm::Matrix<pm::Rational>&, pm::all_selector const&, pm::Complement<pm::SingleElementSet<int>, int, pm::operations::cmp> const&> const&, pm::SingleCol<pm::Vector<pm::Rational> const&> > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
