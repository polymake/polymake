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
#include "polymake/Rational.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/linalg.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/Integer.h"
#include "polymake/RationalFunction.h"
#include "polymake/Set.h"

namespace polymake { namespace common { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1>
   FunctionInterface4perl( new_X, T0,T1 ) {
      perl::Value arg0(stack[1]);
      WrapperReturnNew(T0, (arg0.get<T1>()) );
   };

   FunctionInstance4perl(new_X, SparseMatrix< Rational, Symmetric >, perl::Canned< const SparseMatrix< Rational, Symmetric > >);
   FunctionInstance4perl(new_X, SparseMatrix< Rational, NonSymmetric >, perl::Canned< const SparseMatrix< Rational, Symmetric > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Wary< SparseMatrix< double, NonSymmetric > > >, perl::Canned< const SparseMatrix< double, NonSymmetric > >);
   FunctionInstance4perl(new_X, SparseMatrix< Rational, NonSymmetric >, perl::Canned< const pm::RowChain<pm::Matrix<pm::Rational> const&, pm::SingleRow<pm::SameElementSparseVector<pm::SingleElementSet<int>, pm::Rational> const&> > >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Wary< SparseMatrix< Rational, NonSymmetric > > >, perl::Canned< const Vector< Rational > >);
   OperatorInstance4perl(Binary__ne, perl::Canned< const Wary< SparseMatrix< Rational, NonSymmetric > > >, perl::Canned< const Matrix<Rational> >);
   FunctionInstance4perl(new_X, SparseMatrix< double, NonSymmetric >, perl::Canned< const SparseMatrix< double, NonSymmetric > >);
   OperatorInstance4perl(Binary_mul, int, perl::Canned< const Wary< pm::DiagMatrix<pm::SameElementVector<pm::Rational const&>, true> > >);
   FunctionInstance4perl(new_X, SparseMatrix< Rational, NonSymmetric >, perl::Canned< const pm::DiagMatrix<pm::SameElementVector<pm::Rational const&>, true> >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Wary< SparseMatrix< Integer, NonSymmetric > > >, perl::Canned< const Vector< Integer > >);
   OperatorInstance4perl(BinaryAssign_div, perl::Canned< Wary< SparseMatrix< Integer, NonSymmetric > > >, perl::Canned< const pm::RowChain<pm::Matrix<pm::Integer> const&, pm::Matrix<pm::Integer> const&> >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Wary< SparseMatrix< Integer, NonSymmetric > > >, perl::Canned< const pm::Transposed<pm::Matrix<pm::Integer> > >);
   Class4perl("Polymake::common::SparseMatrix_A_RationalFunction_A_Rational_I_Int_Z_I_Symmetric_Z", SparseMatrix< RationalFunction< Rational, int >, Symmetric >);
   FunctionInstance4perl(new_X, SparseMatrix< Rational, NonSymmetric >, perl::Canned< const SparseMatrix< double, NonSymmetric > >);
   Class4perl("Polymake::common::SparseMatrix_A_Integer_I_Symmetric_Z", SparseMatrix< Integer, Symmetric >);
   FunctionInstance4perl(new_X, SparseMatrix< int, NonSymmetric >, perl::Canned< const SparseMatrix< int, NonSymmetric > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
