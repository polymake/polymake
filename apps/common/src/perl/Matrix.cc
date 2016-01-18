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
#include "polymake/Rational.h"
#include "polymake/client.h"
#include "polymake/linalg.h"

namespace polymake { namespace common { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
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
   OperatorInstance4perl(Binary_sub, perl::Canned< const Wary< Matrix< Integer > > >, perl::Canned< const pm::RepeatedRow<pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Integer>&>, pm::Series<int, true>, void> const&> >);
   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const pm::RowChain<pm::RowChain<pm::Matrix<pm::Rational> const&, pm::Matrix<pm::Rational> const&> const&, pm::Matrix<pm::Rational> const&> >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
