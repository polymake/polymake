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

#include "polymake/IncidenceMatrix.h"
#include "polymake/Integer.h"
#include "polymake/Matrix.h"
#include "polymake/Polynomial.h"
#include "polymake/PuiseuxFraction.h"
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

   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const pm::RowChain<pm::SparseMatrix<pm::Rational, pm::NonSymmetric> const&, pm::SingleRow<pm::Vector<pm::Rational> const&> > >);
   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const pm::RowChain<pm::SparseMatrix<pm::Rational, pm::NonSymmetric> const&, pm::Matrix<pm::Rational> const&> >);
   OperatorInstance4perl(BinaryAssign_div, perl::Canned< Wary< Matrix< PuiseuxFraction< Min, Rational, Rational > > > >, perl::Canned< const Vector< PuiseuxFraction< Min, Rational, Rational > > >);
   OperatorInstance4perl(BinaryAssign_div, perl::Canned< Wary< Matrix< PuiseuxFraction< Min, Rational, Rational > > > >, perl::Canned< const pm::DiagMatrix<pm::SameElementVector<pm::PuiseuxFraction<pm::Min, pm::Rational, pm::Rational> const&>, true> >);
   OperatorInstance4perl(BinaryAssign_div, perl::Canned< Wary< Matrix< PuiseuxFraction< Min, Rational, Rational > > > >, perl::Canned< const pm::MatrixMinor<pm::DiagMatrix<pm::SameElementVector<pm::PuiseuxFraction<pm::Min, pm::Rational, pm::Rational> const&>, true> const&, pm::Complement<pm::SingleElementSetCmp<int, pm::operations::cmp>, int, pm::operations::cmp> const&, pm::all_selector const&> >);
   OperatorInstance4perl(Unary_neg, perl::Canned< const Wary< pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::PuiseuxFraction<pm::Min, pm::Rational, pm::Rational> const&> const&>, pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::PuiseuxFraction<pm::Min, pm::Rational, pm::Rational> const&> const&>, pm::Matrix<pm::PuiseuxFraction<pm::Min, pm::Rational, pm::Rational> > const&> const&> > >);
   OperatorInstance4perl(Binary_diva, perl::Canned< const Wary< Matrix< PuiseuxFraction< Min, Rational, Rational > > > >, perl::Canned< const Vector< PuiseuxFraction< Min, Rational, Rational > > >);
   FunctionInstance4perl(new_X, Matrix< PuiseuxFraction< Min, Rational, Rational > >, perl::Canned< const pm::RowChain<pm::Matrix<pm::PuiseuxFraction<pm::Min, pm::Rational, pm::Rational> > const&, pm::SingleRow<pm::Vector<pm::PuiseuxFraction<pm::Min, pm::Rational, pm::Rational> > const&> > >);
   OperatorInstance4perl(assign, Matrix< TropicalNumber< Min, Rational > >, perl::Canned< const Matrix< Rational > >);
   FunctionInstance4perl(new_X, Matrix< int >, perl::Canned< const pm::MatrixMinor<pm::Matrix<pm::Integer> const&, pm::all_selector const&, pm::Complement<pm::SingleElementSetCmp<int, pm::operations::cmp>, int, pm::operations::cmp> const&> >);
   Class4perl("Polymake::common::Matrix_A_Polynomial_A_Rational_I_Int_Z_I_NonSymmetric_Z", Matrix< Polynomial< Rational, int > >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Wary< Matrix< Polynomial< Rational, int > > > >, perl::Canned< const Vector< Polynomial< Rational, int > > >);
   Class4perl("Polymake::common::Matrix_A_Polynomial_A_QuadraticExtension__Rational_I_Int_Z_I_NonSymmetric_Z", Matrix< Polynomial< QuadraticExtension< Rational >, int > >);
   FunctionInstance4perl(new_int_int, Matrix< Polynomial< QuadraticExtension< Rational >, int > >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Wary< Matrix< Polynomial< QuadraticExtension< Rational >, int > > > >, perl::Canned< const Vector< Polynomial< QuadraticExtension< Rational >, int > > >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Wary< Matrix< QuadraticExtension< Rational > > > >, perl::Canned< const pm::Transposed<pm::SparseMatrix<pm::QuadraticExtension<pm::Rational>, pm::NonSymmetric> > >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Wary< Matrix< PuiseuxFraction< Min, Rational, Rational > > > >, perl::Canned< const SparseVector< PuiseuxFraction< Min, Rational, Rational > > >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Wary< Matrix< QuadraticExtension< Rational > > > >, perl::Canned< const Matrix< QuadraticExtension< Rational > > >);
   FunctionInstance4perl(new_X, Matrix< QuadraticExtension< Rational > >, perl::Canned< const pm::MatrixMinor<pm::Matrix<pm::QuadraticExtension<pm::Rational> >&, pm::all_selector const&, pm::Set<int, pm::operations::cmp> const&> >);
   OperatorInstance4perl(Binary__ora, perl::Canned< const Wary< Matrix< QuadraticExtension< Rational > > > >, perl::Canned< const pm::RepeatedRow<pm::SameElementVector<pm::QuadraticExtension<pm::Rational> const&> > >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Wary< pm::ColChain<pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&, pm::RepeatedRow<pm::SameElementVector<pm::QuadraticExtension<pm::Rational> const&> > const&> > >, perl::Canned< const pm::Transposed<pm::Matrix<pm::QuadraticExtension<pm::Rational> > > >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Wary< pm::ColChain<pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&, pm::RepeatedRow<pm::SameElementVector<pm::QuadraticExtension<pm::Rational> const&> > const&> > >, perl::Canned< const pm::RowChain<pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&, pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&> >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Wary< Matrix< QuadraticExtension< Rational > > > >, perl::Canned< const pm::Transposed<pm::Matrix<pm::Rational> > >);
   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const pm::MatrixMinor<pm::Matrix<pm::QuadraticExtension<pm::Rational> >&, pm::all_selector const&, pm::Set<int, pm::operations::cmp> const&> >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Wary< pm::ColChain<pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&, pm::RepeatedRow<pm::SameElementVector<pm::QuadraticExtension<pm::Rational> const&> > const&> > >, perl::Canned< const Matrix< Rational > >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Wary< pm::ColChain<pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&, pm::RepeatedRow<pm::SameElementVector<pm::QuadraticExtension<pm::Rational> const&> > const&> > >, perl::Canned< const pm::Transposed<pm::Matrix<pm::Rational> > >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Wary< pm::RowChain<pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&, pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&> > >, perl::Canned< const Vector< QuadraticExtension< Rational > > >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Wary< Matrix< QuadraticExtension< Rational > > > >, perl::Canned< const Matrix< Rational > >);
   OperatorInstance4perl(Binary_add, perl::Canned< const Wary< Matrix< TropicalNumber< Max, Rational > > > >, perl::Canned< const Matrix< TropicalNumber< Max, Rational > > >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Wary< Matrix< TropicalNumber< Max, Rational > > > >, perl::Canned< const Vector< TropicalNumber< Max, Rational > > >);
   FunctionInstance4perl(new_X, Matrix< int >, perl::Canned< const pm::ColChain<pm::SingleCol<pm::Vector<int> const&>, pm::Matrix<int> const&> >);
   OperatorInstance4perl(Binary_diva, perl::Canned< const Wary< Matrix< int > > >, perl::Canned< const Matrix< int > >);
   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const pm::RowChain<pm::Matrix<int> const&, pm::Matrix<int> const&> >);
   OperatorInstance4perl(BinaryAssign_add, perl::Canned< Wary< Matrix< Rational > > >, perl::Canned< const Matrix< Rational > >);
   OperatorInstance4perl(Binary_diva, perl::Canned< const Wary< pm::RowChain<pm::Matrix<pm::Rational> const&, pm::SingleRow<pm::Vector<pm::Rational> const&> > > >, perl::Canned< const Matrix< Rational > >);
   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const pm::SingleRow<pm::ContainerUnion<pm::cons<pm::Vector<pm::Rational> const&, pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational> const&>, pm::Series<int, true>, mlist<> > >, void> const&> >);
   OperatorInstance4perl(BinaryAssign_div, perl::Canned< Wary< Matrix< double > > >, perl::Canned< const Vector< double > >);
   OperatorInstance4perl(Binary_mul, int, perl::Canned< const Wary< pm::RepeatedRow<pm::SameElementVector<pm::Rational const&> > > >);
   OperatorInstance4perl(Binary_add, perl::Canned< const Wary< Matrix< Rational > > >, perl::Canned< const pm::DiagMatrix<pm::SameElementVector<pm::Rational const&>, true> >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
