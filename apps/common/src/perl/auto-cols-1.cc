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

///==== this line controls the automatic file splitting: max.instances=40

#include "polymake/client.h"
#include "polymake/Matrix.h"
#include "polymake/Rational.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Vector.h"
#include "polymake/Set.h"
#include "polymake/Integer.h"
#include "polymake/linalg.h"
#include "polymake/SparseMatrix.h"
#include "polymake/Graph.h"
#include "polymake/QuadraticExtension.h"
#include "polymake/Array.h"

namespace polymake { namespace common { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0>
   FunctionInterface4perl( cols_f1, T0 ) {
      perl::Value arg0(stack[0]);
      WrapperReturn( arg0.get<T0>().cols() );
   };

   FunctionInstance4perl(cols_f1, perl::Canned< const Matrix< Rational > >);
   FunctionInstance4perl(cols_f1, perl::Canned< const IncidenceMatrix< NonSymmetric > >);
   FunctionInstance4perl(cols_f1, perl::Canned< const pm::MatrixMinor<pm::Matrix<double>&, pm::Series<int, true> const&, pm::all_selector const&> >);
   FunctionInstance4perl(cols_f1, perl::Canned< const pm::MatrixMinor<pm::MatrixMinor<pm::Matrix<double>&, pm::Series<int, true> const&, pm::all_selector const&>&, pm::Set<int, pm::operations::cmp> const&, pm::all_selector const&> >);
   FunctionInstance4perl(cols_f1, perl::Canned< const Matrix< double > >);
   FunctionInstance4perl(cols_f1, perl::Canned< const pm::MatrixMinor<pm::Matrix<double>&, pm::Set<int, pm::operations::cmp> const&, pm::all_selector const&> >);
   FunctionInstance4perl(cols_f1, perl::Canned< const Matrix< int > >);
   FunctionInstance4perl(cols_f1, perl::Canned< const pm::Transposed<pm::Matrix<pm::Integer> > >);
   FunctionInstance4perl(cols_f1, perl::Canned< const Matrix< Integer > >);
   FunctionInstance4perl(cols_f1, perl::Canned< const pm::RowChain<pm::SingleRow<pm::Vector<double> const&>, pm::Matrix<double> const&> >);
   FunctionInstance4perl(cols_f1, perl::Canned< const pm::MatrixMinor<pm::Matrix<pm::Rational> const&, pm::Complement<pm::Set<int, pm::operations::cmp>, int, pm::operations::cmp> const&, pm::all_selector const&> >);
   FunctionInstance4perl(cols_f1, perl::Canned< const SparseMatrix< double, NonSymmetric > >);
   FunctionInstance4perl(cols_f1, perl::Canned< const SparseMatrix< Rational, NonSymmetric > >);
   FunctionInstance4perl(cols_f1, perl::Canned< const pm::MatrixMinor<pm::SparseMatrix<double, pm::NonSymmetric>&, pm::Set<int, pm::operations::cmp> const&, pm::all_selector const&> >);
   FunctionInstance4perl(cols_f1, perl::Canned< const pm::MatrixMinor<pm::Matrix<pm::Integer>&, pm::incidence_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::nothing, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&> const&, pm::all_selector const&> >);
   FunctionInstance4perl(cols_f1, perl::Canned< const pm::AdjacencyMatrix<pm::graph::Graph<pm::graph::Directed> > >);
   FunctionInstance4perl(cols_f1, perl::Canned< const SparseMatrix< int, NonSymmetric > >);
   FunctionInstance4perl(cols_f1, perl::Canned< const SparseMatrix< Rational, Symmetric > >);
   FunctionInstance4perl(cols_f1, perl::Canned< const SparseMatrix< Integer, NonSymmetric > >);
   FunctionInstance4perl(cols_f1, perl::Canned< const pm::RowChain<pm::RowChain<pm::SparseMatrix<pm::Rational, pm::NonSymmetric> const&, pm::Matrix<pm::Rational> const&> const&, pm::Matrix<pm::Rational> const&> >);
   FunctionInstance4perl(cols_f1, perl::Canned< const pm::RowChain<pm::RowChain<pm::Matrix<pm::Rational> const&, pm::Matrix<pm::Rational> const&> const&, pm::Matrix<pm::Rational> const&> >);
   FunctionInstance4perl(cols_f1, perl::Canned< const pm::ColChain<pm::SingleCol<pm::Vector<pm::Rational> const&> const&, pm::Matrix<pm::Rational> const&> >);
   FunctionInstance4perl(cols_f1, perl::Canned< const pm::RowChain<pm::SingleRow<pm::Vector<pm::Rational> const&> const&, pm::RowChain<pm::RowChain<pm::Matrix<pm::Rational> const&, pm::Matrix<pm::Rational> const&> const&, pm::Matrix<pm::Rational> const&> const&> >);
   FunctionInstance4perl(cols_f1, perl::Canned< const pm::RowChain<pm::SingleRow<pm::Vector<pm::Rational> const&> const&, pm::Matrix<pm::Rational> const&> >);
   FunctionInstance4perl(cols_f1, perl::Canned< const pm::DiagMatrix<pm::SameElementVector<int const&>, true> >);
   FunctionInstance4perl(cols_f1, perl::Canned< const pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::Rational const&> const&>, pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::Rational const&> const&>, pm::Matrix<pm::Rational> const&> const&> >);
   FunctionInstance4perl(cols_f1, perl::Canned< const pm::ColChain<pm::SingleCol<pm::SameElementVector<double const&> const&>, pm::ColChain<pm::SingleCol<pm::SameElementVector<double const&> const&>, pm::Matrix<double> const&> const&> >);
   FunctionInstance4perl(cols_f1, perl::Canned< const pm::AdjacencyMatrix<pm::graph::Graph<pm::graph::DirectedMulti>, true> >);
   FunctionInstance4perl(cols_f1, perl::Canned< const pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::Rational const&> const&>, pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::Rational const&> const&>, pm::SparseMatrix<pm::Rational, pm::NonSymmetric> const&> const&> >);
   FunctionInstance4perl(cols_f1, perl::Canned< const SparseMatrix< QuadraticExtension< Rational >, NonSymmetric > >);
   FunctionInstance4perl(cols_f1, perl::Canned< const pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::QuadraticExtension<pm::Rational> const&> const&>, pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::QuadraticExtension<pm::Rational> const&> const&>, pm::SparseMatrix<pm::QuadraticExtension<pm::Rational>, pm::NonSymmetric> const&> const&> >);
   FunctionInstance4perl(cols_f1, perl::Canned< const Matrix< QuadraticExtension< Rational > > >);
   FunctionInstance4perl(cols_f1, perl::Canned< const pm::ColChain<pm::SingleCol<pm::SameElementVector<double const&> const&>, pm::ColChain<pm::SingleCol<pm::SameElementVector<double const&> const&>, pm::SparseMatrix<double, pm::NonSymmetric> const&> const&> >);
   FunctionInstance4perl(cols_f1, perl::Canned< const pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::QuadraticExtension<pm::Rational> const&> const&>, pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::QuadraticExtension<pm::Rational> const&> const&>, pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&> const&> >);
   FunctionInstance4perl(cols_f1, perl::Canned< const pm::MatrixMinor<pm::Matrix<pm::Integer>&, pm::all_selector const&, pm::Complement<pm::Set<int, pm::operations::cmp>, int, pm::operations::cmp> const&> >);
   FunctionInstance4perl(cols_f1, perl::Canned< const pm::MatrixMinor<pm::Matrix<pm::Rational> const&, pm::all_selector const&, pm::Array<int, void> const&> >);
   FunctionInstance4perl(cols_f1, perl::Canned< const pm::ColChain<pm::Matrix<int> const&, pm::Matrix<int> const&> >);
   FunctionInstance4perl(cols_f1, perl::Canned< const pm::ColChain<pm::SingleCol<pm::Vector<pm::Rational> const&>, pm::Matrix<pm::Rational> const&> >);
   FunctionInstance4perl(cols_f1, perl::Canned< const pm::ColChain<pm::SingleCol<pm::IndexedSlice<pm::Vector<pm::Rational> const&, pm::incidence_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::nothing, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&> const&, void> const&>, pm::MatrixMinor<pm::Matrix<pm::Rational> const&, pm::incidence_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::nothing, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&> const&, pm::all_selector const&> const&> >);
   FunctionInstance4perl(cols_f1, perl::Canned< const pm::Transposed<pm::Matrix<pm::Rational> > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
