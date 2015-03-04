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
#include "polymake/Matrix.h"
#include "polymake/Rational.h"
#include "polymake/linalg.h"
#include "polymake/Set.h"
#include "polymake/Vector.h"
#include "polymake/SparseMatrix.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Array.h"

namespace polymake { namespace common { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1>
   FunctionInterface4perl( new_X, T0,T1 ) {
      perl::Value arg0(stack[1]);
      WrapperReturnNew(T0, (arg0.get<T1>()) );
   };

   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const pm::ColChain<pm::SingleCol<pm::VectorChain<pm::Vector<pm::Rational> const&, pm::IndexedSlice<pm::Vector<pm::Rational>&, pm::Series<int, true>, void> const&> const&>, pm::MatrixMinor<pm::Matrix<pm::Rational>&, pm::Series<int, true> const&, pm::Series<int, true> const&> const&> >);
   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const pm::ColChain<pm::SingleCol<pm::VectorChain<pm::Vector<pm::Rational> const&, pm::IndexedSlice<pm::Vector<pm::Rational>&, pm::Series<int, true>, void> const&> const&>, pm::MatrixMinor<pm::Matrix<pm::Rational>&, pm::Series<int, true> const&, pm::Set<int, pm::operations::cmp> const&> const&> >);
   OperatorInstance4perl(Unary_neg, perl::Canned< const Wary< pm::RowChain<pm::MatrixMinor<pm::Matrix<double>&, pm::incidence_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::nothing, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&> const&, pm::all_selector const&> const&, pm::SingleRow<pm::Vector<double> const&> > > >);
   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const SparseMatrix< Rational, Symmetric > >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Wary< Matrix< double > > >, double);
   OperatorInstance4perl(convert, Matrix< double >, perl::Canned< const SparseMatrix< Rational, NonSymmetric > >);
   OperatorInstance4perl(convert, Matrix< Rational >, perl::Canned< const SparseMatrix< Rational, Symmetric > >);
   FunctionInstance4perl(new_X, Matrix<double>, perl::Canned< const pm::MatrixMinor<pm::Matrix<double>&, pm::incidence_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::nothing, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&> const&, pm::all_selector const&> >);
   OperatorInstance4perl(Unary_neg, perl::Canned< const Wary< Matrix<Rational> > >);
   FunctionInstance4perl(new_X, Matrix<Rational>, perl::Canned< const pm::ColChain<pm::SingleCol<pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational> const&>, pm::Series<int, false>, void> const&>, pm::Matrix<pm::Rational> const&> >);
   FunctionInstance4perl(new_X, Matrix<Rational>, perl::Canned< const pm::ColChain<pm::SingleCol<pm::Vector<pm::Rational> const&>, pm::MatrixMinor<pm::Matrix<pm::Rational> const&, pm::Array<int, void> const&, pm::Series<int, true> const&> const&> >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
