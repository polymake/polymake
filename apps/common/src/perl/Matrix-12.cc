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
#include "polymake/QuadraticExtension.h"
#include "polymake/Rational.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/linalg.h"
#include "polymake/Set.h"
#include "polymake/Vector.h"
#include "polymake/Integer.h"
#include "polymake/SparseMatrix.h"

namespace polymake { namespace common { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1>
   FunctionInterface4perl( new_X, T0,T1 ) {
      perl::Value arg0(stack[1]);
      WrapperReturnNew(T0, (arg0.get<T1>()) );
   };

   FunctionInstance4perl(new_X, Matrix< QuadraticExtension< Rational > >, perl::Canned< const pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::QuadraticExtension<pm::Rational> const&> const&>, pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&> >);
   FunctionInstance4perl(new_X, Matrix< int >, perl::Canned< const pm::DiagMatrix<pm::SameElementVector<pm::Rational const&>, true> >);
   OperatorInstance4perl(Binary_diva, perl::Canned< const Wary< Matrix< Rational > > >, perl::Canned< const pm::MatrixMinor<pm::Matrix<pm::Rational> const&, pm::Set<int, pm::operations::cmp> const&, pm::Series<int, true> const&> >);
   OperatorInstance4perl(Binary_diva, perl::Canned< const Wary< Matrix< Rational > > >, perl::Canned< const Matrix< Rational > >);
   OperatorInstance4perl(Binary_diva, perl::Canned< const Wary< Matrix< Rational > > >, perl::Canned< const Vector< Rational > >);
   OperatorInstance4perl(Binary__ora, perl::Canned< const Wary< Matrix< Rational > > >, perl::Canned< const pm::DiagMatrix<pm::SameElementVector<pm::Rational const&>, true> >);
   OperatorInstance4perl(Binary__ora, perl::Canned< const Wary< pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::Rational const&> const&>, pm::RepeatedRow<pm::SameElementVector<pm::Rational const&> > const&> > >, perl::Canned< const pm::DiagMatrix<pm::SameElementVector<pm::Rational const&>, true> >);
   OperatorInstance4perl(Binary_diva, perl::Canned< const Wary< Matrix< Integer > > >, perl::Canned< const Matrix< Integer > >);
   OperatorInstance4perl(Binary_diva, perl::Canned< const Wary< Matrix< double > > >, perl::Canned< const Matrix< double > >);
   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const pm::MatrixMinor<pm::Matrix<pm::Rational> const&, pm::incidence_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::nothing, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&> const&, pm::Complement<pm::SingleElementSet<int>, int, pm::operations::cmp> const&> >);
   OperatorInstance4perl(Binary_diva, perl::Canned< const Wary< Matrix< Rational > > >, perl::Canned< const SparseMatrix< Rational, NonSymmetric > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Wary< Matrix< Rational > > >, perl::Canned< const pm::MatrixMinor<pm::Matrix<pm::Rational> const&, pm::all_selector const&, pm::Complement<pm::SingleElementSet<int>, int, pm::operations::cmp> const&> >);
   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const pm::MatrixMinor<pm::Matrix<int>&, pm::all_selector const&, pm::Complement<pm::SingleElementSet<int>, int, pm::operations::cmp> const&> >);
   OperatorInstance4perl(Binary__ora, perl::Canned< const Wary< pm::MatrixMinor<pm::Matrix<int>&, pm::all_selector const&, pm::Complement<pm::SingleElementSet<int>, int, pm::operations::cmp> const&> > >, perl::Canned< const Vector< int > >);
   FunctionInstance4perl(new_X, Matrix< int >, perl::Canned< const pm::ColChain<pm::MatrixMinor<pm::Matrix<int>&, pm::all_selector const&, pm::Complement<pm::SingleElementSet<int>, int, pm::operations::cmp> const&> const&, pm::SingleCol<pm::Vector<int> const&> > >);
   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const pm::MatrixMinor<pm::Matrix<pm::Rational>&, pm::Complement<pm::SingleElementSet<int>, int, pm::operations::cmp> const&, pm::all_selector const&> >);
   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const pm::Transposed<pm::SparseMatrix<int, pm::NonSymmetric> > >);
   OperatorInstance4perl(Binary_diva, perl::Canned< const Wary< pm::MatrixMinor<pm::Matrix<double>&, pm::incidence_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::nothing, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&> const&, pm::all_selector const&> > >, perl::Canned< const Vector< double > >);
   OperatorInstance4perl(Binary_diva, perl::Canned< const Wary< Matrix< double > > >, perl::Canned< const Vector< double > >);
   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const pm::MatrixMinor<pm::MatrixMinor<pm::Matrix<pm::Rational> const&, pm::all_selector const&, pm::Complement<pm::SingleElementSet<int>, int, pm::operations::cmp> const&> const&, pm::Set<int, pm::operations::cmp> const&, pm::all_selector const&> >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Wary< Matrix< Rational > > >, perl::Canned< const pm::MatrixMinor<pm::Matrix<pm::Rational> const&, pm::Complement<pm::Set<int, pm::operations::cmp>, int, pm::operations::cmp> const&, pm::Complement<pm::SingleElementSet<int>, int, pm::operations::cmp> const&> >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
