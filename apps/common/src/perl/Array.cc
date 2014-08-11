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
#include "polymake/Array.h"
#include "polymake/Polynomial.h"
#include "polymake/Rational.h"
#include "polymake/Graph.h"
#include "polymake/Set.h"
#include "polymake/Vector.h"
#include "polymake/Matrix.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Integer.h"

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

   Class4perl("Polymake::common::Array__Set__Array__Set__Int", Array< Set< Array< Set< int > > > >);
   FunctionInstance4perl(new_X, Array< Array< Set< int > > >, perl::Canned< const Array< Set< Set< int > > > >);
   FunctionInstance4perl(new_X, Array< Set< Array< Set< int > > > >, perl::Canned< const Array< Set< Set< Set< int > > > > >);
   FunctionInstance4perl(new, Array< Set< Array< Set< int > > > >);
   OperatorInstance4perl(convert, Array< int >, perl::Canned< const pm::Series<int, true> >);
   Class4perl("Polymake::common::Array__Vector__Rational", Array< Vector< Rational > >);
   OperatorInstance4perl(assign, Array< Matrix< Rational > >, perl::Canned< const Array< Matrix< Integer > > >);
   FunctionInstance4perl(new, Array< Vector< Rational > >);
   OperatorInstance4perl(convert, Array< int >, perl::Canned< const Set< int > >);
   OperatorInstance4perl(assign, Array< int >, perl::Canned< const pm::cascaded_iterator<pm::unary_transform_iterator<pm::unary_transform_iterator<pm::graph::valid_node_iterator<pm::iterator_range<pm::graph::node_entry<pm::graph::Undirected, (pm::sparse2d::restriction_kind)0> const*>, pm::BuildUnary<pm::graph::valid_node_selector> >, pm::graph::line_factory<true, pm::graph::incident_edge_list, void> >, pm::operations::masquerade<pm::graph::uniq_edge_list> >, pm::end_sensitive, 2> >);
   FunctionInstance4perl(new_X, Array< Polynomial< Rational, int > >, perl::Canned< const Array< Polynomial< Rational, int > > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
