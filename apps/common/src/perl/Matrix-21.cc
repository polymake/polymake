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
#include "polymake/PuiseuxFraction.h"
#include "polymake/QuadraticExtension.h"
#include "polymake/Rational.h"
#include "polymake/RationalFunction.h"
#include "polymake/Set.h"
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
   FunctionInterface4perl( new, T0 ) {
      WrapperReturnNew(T0, () );
   };

   template <typename T0>
   FunctionInterface4perl( new_int_int, T0 ) {
      perl::Value arg0(stack[1]), arg1(stack[2]);
      WrapperReturnNew(T0, (arg0.get<int>(), arg1.get<int>()) );
   };

   OperatorInstance4perl(Binary_mul, perl::Canned< const Wary< Matrix< Rational > > >, perl::Canned< const pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational> const&>, pm::Series<int, true>, mlist<> > >);
   OperatorInstance4perl(BinaryAssign_mul, perl::Canned< Wary< pm::MatrixMinor<pm::Matrix<pm::Rational>&, pm::all_selector const&, pm::Complement<pm::SingleElementSetCmp<int, pm::operations::cmp>, int, pm::operations::cmp> const&> > >, int);
   FunctionInstance4perl(new_X, Matrix< Integer >, perl::Canned< const pm::MatrixMinor<pm::Matrix<int> const&, pm::Set<int, pm::operations::cmp> const&, pm::all_selector const&> >);
   FunctionInstance4perl(new_X, Matrix< Integer >, perl::Canned< const pm::MatrixMinor<pm::SparseMatrix<int, pm::NonSymmetric> const&, pm::Set<int, pm::operations::cmp> const&, pm::all_selector const&> >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Wary< Matrix< QuadraticExtension< Rational > > > >, perl::Canned< const pm::SameElementSparseVector<pm::SingleElementSetCmp<int, pm::operations::cmp>, pm::Rational> >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Wary< Matrix< QuadraticExtension< Rational > > > >, perl::Canned< const pm::SameElementSparseVector<pm::SingleElementSetCmp<int, pm::operations::cmp>, pm::QuadraticExtension<pm::Rational> > >);
   FunctionInstance4perl(new_X, Matrix< double >, perl::Canned< const pm::ColChain<pm::SingleCol<pm::SameElementVector<double const&> const&>, pm::RowChain<pm::Matrix<double> const&, pm::SingleRow<pm::Vector<double> const&> > const&> >);
   Class4perl("Polymake::common::Matrix_A_PuiseuxFraction_A_Min_I_Rational_I_Rational_Z_I_NonSymmetric_Z", Matrix< PuiseuxFraction< Min, Rational, Rational > >);
   FunctionInstance4perl(new_X, Matrix< PuiseuxFraction< Min, Rational, Rational > >, perl::Canned< const Matrix< PuiseuxFraction< Min, Rational, Rational > > >);
   FunctionInstance4perl(new, Matrix< PuiseuxFraction< Min, Rational, Rational > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Wary< Matrix< PuiseuxFraction< Min, Rational, Rational > > > >, perl::Canned< const Matrix< PuiseuxFraction< Min, Rational, Rational > > >);
   Class4perl("Polymake::common::Matrix_A_PuiseuxFraction_A_Max_I_Rational_I_Rational_Z_I_NonSymmetric_Z", Matrix< PuiseuxFraction< Max, Rational, Rational > >);
   FunctionInstance4perl(new, Matrix< PuiseuxFraction< Max, Rational, Rational > >);
   Class4perl("Polymake::common::Matrix_A_UniPolynomial_A_Rational_I_Int_Z_I_NonSymmetric_Z", Matrix< UniPolynomial< Rational, int > >);
   FunctionInstance4perl(new_int_int, Matrix< UniPolynomial< Rational, int > >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Wary< Matrix< PuiseuxFraction< Max, Rational, Rational > > > >, perl::Canned< const Matrix< PuiseuxFraction< Max, Rational, Rational > > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Wary< Matrix< PuiseuxFraction< Max, Rational, Rational > > > >, perl::Canned< const pm::DiagMatrix<pm::SameElementVector<pm::PuiseuxFraction<pm::Max, pm::Rational, pm::Rational> const&>, true> >);
   Class4perl("Polymake::common::Matrix_A_RationalFunction_A_Rational_I_Int_Z_I_NonSymmetric_Z", Matrix< RationalFunction< Rational, int > >);
   FunctionInstance4perl(new, Matrix< RationalFunction< Rational, int > >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Wary< Matrix< RationalFunction< Rational, int > > > >, perl::Canned< const Matrix< RationalFunction< Rational, int > > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Wary< Matrix< RationalFunction< Rational, int > > > >, perl::Canned< const pm::DiagMatrix<pm::SameElementVector<pm::RationalFunction<pm::Rational, int> const&>, true> >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Wary< Matrix< PuiseuxFraction< Max, Rational, Rational > > > >, perl::Canned< const Matrix< PuiseuxFraction< Max, Rational, Rational > > >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Wary< Matrix< PuiseuxFraction< Min, Rational, Rational > > > >, perl::Canned< const Vector< PuiseuxFraction< Min, Rational, Rational > > >);
   FunctionInstance4perl(new_int_int, Matrix< PuiseuxFraction< Min, Rational, Rational > >);
   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::Rational const&> const&>, pm::DiagMatrix<pm::SameElementVector<pm::Rational const&>, true> const&> >);
   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const pm::ColChain<pm::SingleCol<pm::Vector<pm::Rational> const&>, pm::MatrixMinor<pm::Matrix<pm::Rational> const&, pm::all_selector const&, pm::Complement<pm::SingleElementSetCmp<int, pm::operations::cmp>, int, pm::operations::cmp> const&> const&> >);
   OperatorInstance4perl(Binary_diva, perl::Canned< const Wary< pm::MatrixMinor<pm::Matrix<pm::Rational> const&, pm::all_selector const&, pm::Complement<pm::SingleElementSetCmp<int, pm::operations::cmp>, int, pm::operations::cmp> const&> > >, perl::Canned< const pm::DiagMatrix<pm::SameElementVector<pm::Rational const&>, true> >);
   OperatorInstance4perl(convert, Matrix< Integer >, perl::Canned< const SparseMatrix< Rational, NonSymmetric > >);
   FunctionInstance4perl(new_X, Matrix< double >, perl::Canned< const pm::MatrixMinor<pm::SparseMatrix<pm::QuadraticExtension<pm::Rational>, pm::NonSymmetric> const&, pm::all_selector const&, pm::Complement<pm::SingleElementSetCmp<int, pm::operations::cmp>, int, pm::operations::cmp> const&> >);
   OperatorInstance4perl(Binary_mul, int, perl::Canned< const Wary< pm::MatrixMinor<pm::Matrix<pm::Rational> const&, pm::all_selector const&, pm::Complement<pm::SingleElementSetCmp<int, pm::operations::cmp>, int, pm::operations::cmp> const&> > >);
   OperatorInstance4perl(Binary_diva, perl::Canned< const Wary< pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::Rational const&> const&>, pm::Matrix<pm::Rational> const&> > >, perl::Canned< const pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::Rational const&> const&>, pm::Matrix<pm::Rational> const&> >);
   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::Rational const&> const&>, pm::RowChain<pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::Rational const&> const&>, pm::Matrix<pm::Rational> const&> const&, pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::Rational const&> const&>, pm::Matrix<pm::Rational> const&> const&> const&> >);
   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const pm::MatrixMinor<pm::Matrix<pm::Integer> const&, pm::Set<int, pm::operations::cmp> const&, pm::all_selector const&> >);
   OperatorInstance4perl(Binary_diva, perl::Canned< const Wary< pm::SingleRow<pm::Vector<pm::Rational> const&> > >, perl::Canned< const Matrix< Rational > >);
   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const pm::RowChain<pm::SingleRow<pm::Vector<pm::Rational> const&> const&, pm::Matrix<pm::Rational> const&> >);
   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const pm::SingleRow<pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational>&>, pm::Series<int, true>, mlist<> > const&> >);
   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const pm::DiagMatrix<pm::Vector<pm::Rational> const&, false> >);
   FunctionInstance4perl(new_int_int, Matrix< PuiseuxFraction< Max, Rational, Rational > >);
   OperatorInstance4perl(convert, Matrix< PuiseuxFraction< Max, Rational, Rational > >, perl::Canned< const SparseMatrix< PuiseuxFraction< Max, Rational, Rational >, NonSymmetric > >);
   FunctionInstance4perl(new_X, Matrix< PuiseuxFraction< Max, Rational, Rational > >, perl::Canned< const Matrix< PuiseuxFraction< Max, Rational, Rational > > >);
   FunctionInstance4perl(new_X, Matrix< QuadraticExtension< Rational > >, perl::Canned< const pm::SingleRow<pm::Vector<pm::QuadraticExtension<pm::Rational> > const&> >);
   FunctionInstance4perl(new_X, Matrix< TropicalNumber< Min, Rational > >, perl::Canned< const Matrix< Rational > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
