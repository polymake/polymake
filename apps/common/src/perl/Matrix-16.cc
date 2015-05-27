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
#include "polymake/Matrix.h"
#include "polymake/QuadraticExtension.h"
#include "polymake/Rational.h"
#include "polymake/Set.h"
#include "polymake/SparseMatrix.h"
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

   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const pm::RowChain<pm::ColChain<pm::DiagMatrix<pm::SameElementVector<pm::Rational const&>, true> const&, pm::SingleCol<pm::SameElementVector<pm::Rational const&> const&> > const&, pm::Matrix<pm::Rational> const&> >);
   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const pm::ColChain<pm::SingleCol<pm::SameElementSparseVector<pm::SingleElementSet<int>, pm::Rational> const&>, pm::RowChain<pm::DiagMatrix<pm::SameElementVector<pm::Rational const&>, true> const&, pm::MatrixMinor<pm::Matrix<pm::Rational> const&, pm::Array<int, void> const&, pm::Complement<pm::SingleElementSet<int>, int, pm::operations::cmp> const&> const&> const&> >);
   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const pm::ColChain<pm::SingleCol<pm::SameElementSparseVector<pm::SingleElementSet<int>, pm::Rational> const&>, pm::RowChain<pm::Matrix<pm::Rational> const&, pm::DiagMatrix<pm::SameElementVector<pm::Rational const&>, true> const&> const&> >);
   FunctionInstance4perl(new_X, Matrix< double >, perl::Canned< const pm::DiagMatrix<pm::SameElementVector<double const&>, true> >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Wary< Matrix< QuadraticExtension< Rational > > > >, perl::Canned< const SparseMatrix< QuadraticExtension< Rational >, NonSymmetric > >);
   FunctionInstance4perl(new_X, Matrix< QuadraticExtension< Rational > >, perl::Canned< const pm::DiagMatrix<pm::SameElementVector<pm::QuadraticExtension<pm::Rational> const&>, true> >);
   OperatorInstance4perl(Binary_diva, perl::Canned< const Wary< Matrix< Rational > > >, perl::Canned< const pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::Rational const&> const&>, pm::Matrix<pm::Rational> const&> >);
   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const pm::RowChain<pm::Matrix<pm::Rational> const&, pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::Rational const&> const&>, pm::Matrix<pm::Rational> const&> const&> >);
   OperatorInstance4perl(Binary_diva, perl::Canned< const Wary< Matrix< Rational > > >, perl::Canned< const pm::VectorChain<pm::Vector<pm::Rational> const&, pm::SameElementVector<pm::Rational const&> const&> >);
   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const pm::RowChain<pm::Matrix<pm::Rational> const&, pm::SingleRow<pm::VectorChain<pm::Vector<pm::Rational> const&, pm::SameElementVector<pm::Rational const&> const&> const&> > >);
   OperatorInstance4perl(Unary_neg, perl::Canned< const Wary< pm::ColChain<pm::SingleCol<pm::Vector<pm::Rational> const&>, pm::Matrix<pm::Rational> const&> > >);
   OperatorInstance4perl(Binary__ora, perl::Canned< const Wary< Matrix< Rational > > >, perl::Canned< const Vector< Rational > >);
   OperatorInstance4perl(Binary_diva, perl::Canned< const Wary< Matrix< Rational > > >, perl::Canned< const pm::VectorChain<pm::SingleElementVector<pm::Rational>, pm::Vector<pm::Rational> const&> >);
   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const pm::RowChain<pm::Matrix<pm::Rational> const&, pm::SingleRow<pm::VectorChain<pm::SingleElementVector<pm::Rational>, pm::Vector<pm::Rational> const&> const&> > >);
   OperatorInstance4perl(convert, Matrix< int >, perl::Canned< const SparseMatrix< int, NonSymmetric > >);
   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const pm::MatrixMinor<pm::Matrix<pm::Rational> const&, pm::all_selector const&, pm::Complement<pm::SingleElementSet<int>, int, pm::operations::cmp> const&> >);
   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const pm::MatrixMinor<pm::Matrix<pm::Rational> const&, pm::incidence_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::nothing, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&> const&, pm::Series<int, true> const&> >);
   OperatorInstance4perl(Binary__ora, perl::Canned< const Wary< pm::MatrixMinor<pm::Matrix<pm::Rational> const&, pm::all_selector const&, pm::Series<int, true> const&> > >, perl::Canned< const Vector< Rational > >);
   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const pm::ColChain<pm::MatrixMinor<pm::Matrix<pm::Rational> const&, pm::all_selector const&, pm::Series<int, true> const&> const&, pm::SingleCol<pm::Vector<pm::Rational> const&> > >);
   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const pm::MatrixMinor<pm::Matrix<pm::Rational> const&, pm::Set<int, pm::operations::cmp> const&, pm::Complement<pm::SingleElementSet<int>, int, pm::operations::cmp> const&> >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
