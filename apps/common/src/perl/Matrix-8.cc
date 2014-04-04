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
#include "polymake/Matrix.h"
#include "polymake/Rational.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Integer.h"
#include "polymake/linalg.h"
#include "polymake/Vector.h"
#include "polymake/RationalFunction.h"
#include "polymake/SparseMatrix.h"
#include "polymake/Set.h"
#include "polymake/Array.h"

namespace polymake { namespace common { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0>
   FunctionInterface4perl( new, T0 ) {
      WrapperReturnNew(T0, () );
   };

   template <typename T0, typename T1>
   FunctionInterface4perl( new_X, T0,T1 ) {
      perl::Value arg0(stack[1]);
      WrapperReturnNew(T0, (arg0.get<T1>()) );
   };

   OperatorInstance4perl(Binary__ne, perl::Canned< const Wary< Matrix<Integer> > >, perl::Canned< const Matrix<Integer> >);
   OperatorInstance4perl(Binary__ne, perl::Canned< const Wary< Matrix<Rational> > >, perl::Canned< const Matrix<Rational> >);
   OperatorInstance4perl(Binary__ne, perl::Canned< const Wary< Matrix<double> > >, perl::Canned< const Matrix<double> >);
   OperatorInstance4perl(Binary__ne, perl::Canned< const Wary< Matrix<int> > >, perl::Canned< const Matrix<int> >);
   OperatorInstance4perl(BinaryAssign_div, perl::Canned< Wary< Matrix<double> > >, perl::Canned< const Matrix<double> >);
   FunctionInstance4perl(new_X, Matrix<Integer>, perl::Canned< const pm::DiagMatrix<pm::SameElementVector<int const&>, true> >);
   OperatorInstance4perl(Unary_neg, perl::Canned< const Wary< pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::Rational const&> const&>, pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::Rational const&> const&>, pm::Matrix<pm::Rational> const&> const&> > >);
   FunctionInstance4perl(new_X, Matrix<Rational>, perl::Canned< const pm::DiagMatrix<pm::SameElementVector<pm::Rational const&>, true> >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Wary< Matrix<Integer> > >, perl::Canned< const Vector< Integer > >);
   FunctionInstance4perl(new_X, Matrix<Rational>, perl::Canned< const pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::Integer const&> const&>, pm::Matrix<pm::Integer> const&> >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Wary< Matrix< RationalFunction< Rational, int > > > >, perl::Canned< const pm::DiagMatrix<pm::SameElementVector<pm::RationalFunction<pm::Rational, int> const&>, true> >);
   Class4perl("Polymake::common::Matrix_A_RationalFunction_A_Rational_I_Int_Z_I_NonSymmetric_Z", Matrix< RationalFunction< Rational, int > >);
   FunctionInstance4perl(new, Matrix< RationalFunction< Rational, int > >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Wary< Matrix< RationalFunction< Rational, int > > > >, perl::Canned< const Matrix< RationalFunction< Rational, int > > >);
   FunctionInstance4perl(new_X, Matrix< Integer >, perl::Canned< const SparseMatrix< Integer, NonSymmetric > >);
   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const pm::MatrixMinor<pm::Matrix<pm::Rational> const&, pm::Complement<pm::Set<int, pm::operations::cmp>, int, pm::operations::cmp> const&, pm::all_selector const&> >);
   OperatorInstance4perl(Unary_neg, perl::Canned< const Wary< pm::ColChain<pm::SingleCol<pm::SameElementVector<double const&> const&>, pm::ColChain<pm::SingleCol<pm::SameElementVector<double const&> const&>, pm::Matrix<double> const&> const&> > >);
   OperatorInstance4perl(convert, Matrix< Rational >, perl::Canned< const SparseMatrix< double, NonSymmetric > >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Wary< pm::ColChain<pm::SingleCol<pm::SameElementVector<double const&> const&>, pm::Matrix<double> const&> > >, perl::Canned< const pm::RowChain<pm::SingleRow<pm::VectorChain<pm::SingleElementVector<double>, pm::Vector<double> const&> const&>, pm::Matrix<double> const&> >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Wary< Matrix< double > > >, perl::Canned< const Vector< double > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
