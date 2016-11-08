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

#include "polymake/IncidenceMatrix.h"
#include "polymake/Integer.h"
#include "polymake/Matrix.h"
#include "polymake/PuiseuxFraction.h"
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

   OperatorInstance4perl(Binary_mul, perl::Canned< const Wary< Matrix< Rational > > >, perl::Canned< const pm::SameElementSparseVector<pm::SingleElementSet<int>, pm::Rational> >);
   template <typename T0, typename T1>
   FunctionInterface4perl( new_X, T0,T1 ) {
      perl::Value arg0(stack[1]);
      WrapperReturnNew(T0, (arg0.get<T1>()) );
   };



   OperatorInstance4perl(Binary_diva, perl::Canned< const Wary< pm::RowChain<pm::RowChain<pm::RowChain<pm::Matrix<pm::Rational> const&, pm::Matrix<pm::Rational> const&> const&, pm::Matrix<pm::Rational> const&> const&, pm::Matrix<pm::Rational> const&> > >, perl::Canned< const Matrix< Rational > >);
   OperatorInstance4perl(Binary_diva, perl::Canned< const Wary< pm::RowChain<pm::RowChain<pm::RowChain<pm::RowChain<pm::Matrix<pm::Rational> const&, pm::Matrix<pm::Rational> const&> const&, pm::Matrix<pm::Rational> const&> const&, pm::Matrix<pm::Rational> const&> const&, pm::Matrix<pm::Rational> const&> > >, perl::Canned< const Matrix< Rational > >);
   OperatorInstance4perl(Binary_diva, perl::Canned< const Wary< pm::RowChain<pm::RowChain<pm::RowChain<pm::RowChain<pm::RowChain<pm::Matrix<pm::Rational> const&, pm::Matrix<pm::Rational> const&> const&, pm::Matrix<pm::Rational> const&> const&, pm::Matrix<pm::Rational> const&> const&, pm::Matrix<pm::Rational> const&> const&, pm::Matrix<pm::Rational> const&> > >, perl::Canned< const Matrix< Rational > >);
   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::Rational const&> const&>, pm::RowChain<pm::RowChain<pm::RowChain<pm::RowChain<pm::RowChain<pm::RowChain<pm::Matrix<pm::Rational> const&, pm::Matrix<pm::Rational> const&> const&, pm::Matrix<pm::Rational> const&> const&, pm::Matrix<pm::Rational> const&> const&, pm::Matrix<pm::Rational> const&> const&, pm::Matrix<pm::Rational> const&> const&, pm::Matrix<pm::Rational> const&> const&> >);
   FunctionInstance4perl(new_X, Matrix< double >, perl::Canned< const pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::Rational const&> const&>, pm::RowChain<pm::RowChain<pm::RowChain<pm::Matrix<pm::Rational> const&, pm::Matrix<pm::Rational> const&> const&, pm::Matrix<pm::Rational> const&> const&, pm::Matrix<pm::Rational> const&> const&> >);
   FunctionInstance4perl(new_X, Matrix< double >, perl::Canned< const pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::Rational const&> const&>, pm::RowChain<pm::RowChain<pm::RowChain<pm::RowChain<pm::RowChain<pm::RowChain<pm::Matrix<pm::Rational> const&, pm::Matrix<pm::Rational> const&> const&, pm::Matrix<pm::Rational> const&> const&, pm::Matrix<pm::Rational> const&> const&, pm::Matrix<pm::Rational> const&> const&, pm::Matrix<pm::Rational> const&> const&, pm::Matrix<pm::Rational> const&> const&> >);
   OperatorInstance4perl(Binary_sub, perl::Canned< const Wary< Matrix< Integer > > >, perl::Canned< const pm::RepeatedRow<pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Integer>&>, pm::Series<int, true>> const&> >);
   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const pm::RowChain<pm::RowChain<pm::Matrix<pm::Rational> const&, pm::Matrix<pm::Rational> const&> const&, pm::Matrix<pm::Rational> const&> >);
   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const pm::SingleRow<pm::SameElementVector<pm::Rational const&> const&> >);
   OperatorInstance4perl(Binary_diva, perl::Canned< const Wary< pm::RowChain<pm::Matrix<double> const&, pm::Matrix<double> const&> > >, perl::Canned< const Matrix< double > >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Wary< Matrix< double > > >, perl::Canned< const pm::MatrixMinor<pm::Matrix<double>&, pm::Complement<pm::SingleElementSet<int>, int, pm::operations::cmp> const&, pm::Complement<pm::SingleElementSet<int>, int, pm::operations::cmp> const&> >);
   OperatorInstance4perl(Binary_sub, perl::Canned< const Wary< Matrix< Rational > > >, perl::Canned< const pm::RepeatedRow<pm::Vector<pm::Rational> const&> >);
   OperatorInstance4perl(Binary_sub, perl::Canned< const Wary< Matrix< QuadraticExtension< Rational > > > >, perl::Canned< const pm::RepeatedRow<pm::Vector<pm::QuadraticExtension<pm::Rational> > const&> >);
   OperatorInstance4perl(Binary_sub, perl::Canned< const Wary< Matrix< double > > >, perl::Canned< const pm::RepeatedRow<pm::Vector<double> const&> >);
   FunctionInstance4perl(new_int_int, Matrix< PuiseuxFraction< Min, Rational, Rational > >);
   OperatorInstance4perl(BinaryAssign_div, perl::Canned< Wary< Matrix< Rational > > >, perl::Canned< const SparseMatrix< Rational, NonSymmetric > >);
   Class4perl("Polymake::common::Matrix_A_TropicalNumber_A_Min_I_Int_Z_I_NonSymmetric_Z", Matrix< TropicalNumber< Min, int > >);
   FunctionInstance4perl(new_X, Matrix< TropicalNumber< Min, int > >, perl::Canned< const pm::DiagMatrix<pm::SameElementVector<pm::TropicalNumber<pm::Min, int> const&>, true> >);
   FunctionInstance4perl(new_int_int, Matrix< TropicalNumber< Min, int > >);
   OperatorInstance4perl(Binary_add, perl::Canned< const Wary< Matrix< TropicalNumber< Min, int > > > >, perl::Canned< const Matrix< TropicalNumber< Min, int > > >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Wary< Matrix< TropicalNumber< Min, int > > > >, perl::Canned< const Matrix< TropicalNumber< Min, int > > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
