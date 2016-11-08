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

#include "polymake/Array.h"
#include "polymake/FacetList.h"
#include "polymake/Graph.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Matrix.h"
#include "polymake/Rational.h"
#include "polymake/Set.h"
#include "polymake/SparseMatrix.h"
#include "polymake/Vector.h"
#include "polymake/client.h"
#include "polymake/linalg.h"
#include "polymake/list"

namespace polymake { namespace common { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1>
   FunctionInterface4perl( new_X_int, T0,T1 ) {
      perl::Value arg0(stack[1]), arg1(stack[2]);
      WrapperReturnNew(T0, (arg0.get<T1>(), arg1.get<int>()) );
   };

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

   template <typename T0>
   FunctionInterface4perl( new, T0 ) {
      WrapperReturnNew(T0, () );
   };

   ClassTemplate4perl("Polymake::common::IncidenceMatrix");
   Class4perl("Polymake::common::IncidenceMatrix__NonSymmetric", IncidenceMatrix< NonSymmetric >);
   Class4perl("Polymake::common::IncidenceMatrix__Symmetric", IncidenceMatrix< Symmetric >);
   FunctionInstance4perl(new, IncidenceMatrix< NonSymmetric >);
   FunctionInstance4perl(new_X, IncidenceMatrix< NonSymmetric >, perl::Canned< const std::list< Set< int > > >);
   FunctionInstance4perl(new_X, IncidenceMatrix< NonSymmetric >, perl::Canned< const IncidenceMatrix< NonSymmetric > >);
   FunctionInstance4perl(new_X, IncidenceMatrix< NonSymmetric >, perl::Canned< const pm::Transposed<pm::IncidenceMatrix<pm::NonSymmetric> > >);
   OperatorInstance4perl(convert, IncidenceMatrix< NonSymmetric >, perl::Canned< const Array< Set< int > > >);
   FunctionInstance4perl(new_X, IncidenceMatrix< NonSymmetric >, perl::Canned< const FacetList >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Wary< IncidenceMatrix< NonSymmetric > > >, perl::Canned< const IncidenceMatrix< NonSymmetric > >);
   OperatorInstance4perl(Unary_com, perl::Canned< const Wary< IncidenceMatrix< NonSymmetric > > >);
   FunctionInstance4perl(new_X, IncidenceMatrix< NonSymmetric >, perl::Canned< const Array< Set< int > > >);
   OperatorInstance4perl(assign, IncidenceMatrix< NonSymmetric >, perl::Canned< const pm::MatrixMinor<pm::IncidenceMatrix<pm::NonSymmetric> const&, pm::incidence_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::nothing, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&> const&, pm::Set<int, pm::operations::cmp> const&> >);
   OperatorInstance4perl(assign, IncidenceMatrix< NonSymmetric >, perl::Canned< const pm::MatrixMinor<pm::IncidenceMatrix<pm::NonSymmetric> const&, pm::Set<int, pm::operations::cmp> const&, pm::all_selector const&> >);
   OperatorInstance4perl(assign, IncidenceMatrix< NonSymmetric >, perl::Canned< const pm::MatrixMinor<pm::IncidenceMatrix<pm::NonSymmetric>&, pm::Indices<pm::sparse_matrix_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<int, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&, pm::NonSymmetric> const&> const&, pm::all_selector const&>
 >);
   FunctionInstance4perl(new_int_int, IncidenceMatrix< NonSymmetric >);
   OperatorInstance4perl(BinaryAssign_div, perl::Canned< Wary< IncidenceMatrix< NonSymmetric > > >, perl::Canned< const pm::incidence_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::nothing, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&> >);
   FunctionInstance4perl(new_X_int, IncidenceMatrix< NonSymmetric >, perl::Canned< const Array< Set< int > > >);
   OperatorInstance4perl(Binary__ne, perl::Canned< const Wary< IncidenceMatrix< NonSymmetric > > >, perl::Canned< const IncidenceMatrix< NonSymmetric > >);
   FunctionInstance4perl(new_X, IncidenceMatrix< NonSymmetric >, perl::Canned< const pm::MatrixMinor<pm::Transposed<pm::IncidenceMatrix<pm::NonSymmetric> >&, pm::Complement<pm::Set<int, pm::operations::cmp>, int, pm::operations::cmp> const&, pm::all_selector const&> >);
   FunctionInstance4perl(new_X, IncidenceMatrix< NonSymmetric >, perl::Canned< const Array< Array< int > > >);
   OperatorInstance4perl(Unary_com, perl::Canned< const Wary< pm::AdjacencyMatrix<pm::graph::Graph<pm::graph::Undirected>, false> > >);
   OperatorInstance4perl(Binary_sub, perl::Canned< const Wary< pm::ComplementIncidenceMatrix<pm::AdjacencyMatrix<pm::graph::Graph<pm::graph::Undirected>, false> > > >, perl::Canned< const pm::IndexMatrix<pm::DiagMatrix<pm::SameElementVector<pm::Rational const&>, true> const&> >);
   FunctionInstance4perl(new_X, IncidenceMatrix< NonSymmetric >, perl::Canned< const Vector< Set< int > > >);
   OperatorInstance4perl(Binary_diva, perl::Canned< const Wary< IncidenceMatrix< NonSymmetric > > >, perl::Canned< const IncidenceMatrix< NonSymmetric > >);
   FunctionInstance4perl(new_X, IncidenceMatrix< NonSymmetric >, perl::Canned< const pm::RowChain<pm::IncidenceMatrix<pm::NonSymmetric> const&, pm::IncidenceMatrix<pm::NonSymmetric> const&> >);
   FunctionInstance4perl(new_X, IncidenceMatrix< NonSymmetric >, perl::Canned< const pm::MatrixMinor<pm::IncidenceMatrix<pm::NonSymmetric> const&, pm::Set<int, pm::operations::cmp> const&, pm::all_selector const&> >);
   FunctionInstance4perl(new_X, IncidenceMatrix< NonSymmetric >, perl::Canned< const pm::MatrixMinor<pm::IncidenceMatrix<pm::NonSymmetric> const&, pm::all_selector const&, pm::Complement<pm::SingleElementSet<int>, int, pm::operations::cmp> const&> >);
   OperatorInstance4perl(Unary_com, perl::Canned< const Wary< pm::Transposed<pm::IncidenceMatrix<pm::NonSymmetric> > > >);
   FunctionInstance4perl(new_X, IncidenceMatrix< NonSymmetric >, perl::Canned< const pm::ComplementIncidenceMatrix<pm::Transposed<pm::IncidenceMatrix<pm::NonSymmetric> > > >);
   OperatorInstance4perl(convert, IncidenceMatrix< NonSymmetric >, perl::Canned< const NodeMap< Directed, Set< int > > >);
   FunctionInstance4perl(new_X, IncidenceMatrix< NonSymmetric >, perl::Canned< const pm::MatrixMinor<pm::IncidenceMatrix<pm::NonSymmetric> const&, pm::all_selector const&, pm::Set<int, pm::operations::cmp> const&> >);
   OperatorInstance4perl(assign, pm::MatrixMinor<pm::IncidenceMatrix<pm::NonSymmetric>&, pm::Complement<pm::SingleElementSet<int>, int, pm::operations::cmp> const&, pm::Complement<pm::SingleElementSet<int>, int, pm::operations::cmp> const&>, perl::Canned< const pm::MatrixMinor<pm::IncidenceMatrix<pm::NonSymmetric> const&, pm::all_selector const&, pm::incidence_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::nothing, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&> const&> >);
   OperatorInstance4perl(assign, pm::MatrixMinor<pm::IncidenceMatrix<pm::NonSymmetric>&, pm::all_selector const&, pm::Complement<pm::Set<int, pm::operations::cmp>, int, pm::operations::cmp> const&>, perl::Canned< const IncidenceMatrix< NonSymmetric > >);
   FunctionInstance4perl(new_X, IncidenceMatrix< NonSymmetric >, perl::Canned< const pm::MatrixMinor<pm::IncidenceMatrix<pm::NonSymmetric>&, pm::Complement<pm::SingleElementSet<int>, int, pm::operations::cmp> const&, pm::all_selector const&> >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
