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
#include "polymake/Integer.h"
#include "polymake/Rational.h"
#include "polymake/SparseMatrix.h"
#include "polymake/Set.h"
#include "polymake/Vector.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/linalg.h"
#include "polymake/Graph.h"

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

   FunctionInstance4perl(new_X, Matrix< double >, perl::Canned< const Matrix< Rational > >);
   OperatorInstance4perl(convert, Matrix< Integer >, perl::Canned< const Matrix< Rational > >);
   FunctionInstance4perl(new_X, Matrix< double >, perl::Canned< const Matrix< double > >);
   OperatorInstance4perl(assign, Matrix< Rational >, perl::Canned< const SparseMatrix< Rational, NonSymmetric > >);
   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const pm::MatrixMinor<pm::Matrix<pm::Rational> const&, pm::all_selector const&, pm::Series<int, true> const&> >);
   FunctionInstance4perl(new_X, Matrix< Integer >, perl::Canned< const Matrix< Rational > >);
   OperatorInstance4perl(convert, Matrix< Rational >, perl::Canned< const Matrix< Integer > >);
   OperatorInstance4perl(convert, Matrix< int >, perl::Canned< const Matrix< Integer > >);
   OperatorInstance4perl(convert, Matrix< double >, perl::Canned< const Matrix< Rational > >);
   OperatorInstance4perl(assign, Matrix< double >, perl::Canned< const Matrix< Rational > >);
   OperatorInstance4perl(assign, Matrix< double >, perl::Canned< const SparseMatrix< Rational, NonSymmetric > >);
   FunctionInstance4perl(new_int_int, Matrix< double >);
   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const SparseMatrix< Rational, NonSymmetric > >);
   FunctionInstance4perl(new_X, Matrix< double >, perl::Canned< const SparseMatrix< double, NonSymmetric > >);
   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const pm::RowChain<pm::RowChain<pm::MatrixMinor<pm::Matrix<pm::Rational> const&, pm::Set<int, pm::operations::cmp> const&, pm::all_selector const&> const&, pm::SingleRow<pm::Vector<pm::Rational> const&> > const&, pm::SingleRow<pm::Vector<pm::Rational> const&> > >);
   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const pm::RowChain<pm::Matrix<pm::Rational> const&, pm::SingleRow<pm::Vector<pm::Rational> const&> > >);
   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const pm::MatrixMinor<pm::Matrix<pm::Rational> const&, pm::Complement<pm::Set<int, pm::operations::cmp>, int, pm::operations::cmp> const&, pm::Series<int, true> const&> >);
   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const pm::Transposed<pm::Matrix<pm::Rational> > >);
   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const pm::MatrixMinor<pm::Matrix<pm::Rational> const&, pm::Set<int, pm::operations::cmp> const&, pm::all_selector const&> >);
   FunctionInstance4perl(new_X, Matrix< double >, perl::Canned< const NodeMap< Undirected, Vector< Rational > > >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Wary< Matrix< Rational > > >, perl::Canned< const pm::Transposed<pm::Matrix<pm::Rational> > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Wary< Matrix< Integer > > >, perl::Canned< const Matrix< Integer > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Wary< Matrix< Rational > > >, perl::Canned< const Matrix< Rational > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Wary< Matrix< Rational > > >, perl::Canned< const SparseMatrix< Rational, Symmetric > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Wary< Matrix< double > > >, perl::Canned< const Matrix< double > >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Wary< Matrix< double > > >, perl::Canned< const pm::Transposed<pm::Matrix<double> > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Wary< Matrix< Rational > > >, perl::Canned< const SparseMatrix< Rational, NonSymmetric > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Wary< Matrix< int > > >, perl::Canned< const Matrix< int > >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Wary< Matrix< Integer > > >, perl::Canned< const pm::Transposed<pm::Matrix<pm::Integer> > >);
   OperatorInstance4perl(BinaryAssign_div, perl::Canned< Wary< Matrix< Integer > > >, perl::Canned< const Vector< Integer > >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Wary< Matrix< double > > >, perl::Canned< const Matrix< double > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
