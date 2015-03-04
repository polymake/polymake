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

#include "polymake/client.h"
#include "polymake/SparseMatrix.h"
#include "polymake/Rational.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Matrix.h"
#include "polymake/Set.h"
#include "polymake/Array.h"
#include "polymake/linalg.h"
#include "polymake/Vector.h"
#include "polymake/QuadraticExtension.h"

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

   FunctionInstance4perl(new_X, SparseMatrix< Rational, NonSymmetric >, perl::Canned< const pm::RowChain<pm::SparseMatrix<pm::Rational, pm::NonSymmetric> const&, pm::SparseMatrix<pm::Rational, pm::NonSymmetric> const&> >);
   FunctionInstance4perl(new_X, SparseMatrix< Rational, NonSymmetric >, perl::Canned< const pm::MatrixMinor<pm::SparseMatrix<pm::Rational, pm::NonSymmetric> const&, pm::Array<int, void> const&, pm::all_selector const&> >);
   FunctionInstance4perl(new_X, SparseMatrix< Rational, NonSymmetric >, perl::Canned< const pm::RowChain<pm::Matrix<pm::Rational> const&, pm::SparseMatrix<pm::Rational, pm::NonSymmetric> const&> >);
   OperatorInstance4perl(Unary_neg, perl::Canned< const Wary< pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::Rational const&> const&>, pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::Rational const&> const&>, pm::SparseMatrix<pm::Rational, pm::NonSymmetric> const&> const&> > >);
   FunctionInstance4perl(new_X, SparseMatrix< Rational, NonSymmetric >, perl::Canned< const pm::RowChain<pm::SparseMatrix<pm::Rational, pm::NonSymmetric> const&, pm::SingleRow<pm::Vector<pm::Rational> const&> > >);
   Class4perl("Polymake::common::SparseMatrix_A_QuadraticExtension__Rational_I_NonSymmetric_Z", SparseMatrix< QuadraticExtension< Rational >, NonSymmetric >);
   OperatorInstance4perl(Unary_neg, perl::Canned< const Wary< pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::QuadraticExtension<pm::Rational> const&> const&>, pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::QuadraticExtension<pm::Rational> const&> const&>, pm::SparseMatrix<pm::QuadraticExtension<pm::Rational>, pm::NonSymmetric> const&> const&> > >);
   FunctionInstance4perl(new_X, SparseMatrix< QuadraticExtension< Rational >, NonSymmetric >, perl::Canned< const pm::RowChain<pm::SparseMatrix<pm::QuadraticExtension<pm::Rational>, pm::NonSymmetric> const&, pm::SingleRow<pm::Vector<pm::QuadraticExtension<pm::Rational> > const&> > >);
   FunctionInstance4perl(new_X, SparseMatrix< QuadraticExtension< Rational >, NonSymmetric >, perl::Canned< const SparseMatrix< QuadraticExtension< Rational >, NonSymmetric > >);
   FunctionInstance4perl(new, SparseMatrix< QuadraticExtension< Rational >, NonSymmetric >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Wary< SparseMatrix< QuadraticExtension< Rational >, NonSymmetric > > >, perl::Canned< const SparseMatrix< QuadraticExtension< Rational >, NonSymmetric > >);
   OperatorInstance4perl(convert, SparseMatrix< double, NonSymmetric >, perl::Canned< const SparseMatrix< Rational, NonSymmetric > >);
   FunctionInstance4perl(new_X, SparseMatrix< QuadraticExtension< Rational >, NonSymmetric >, perl::Canned< const SparseMatrix< Rational, NonSymmetric > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
