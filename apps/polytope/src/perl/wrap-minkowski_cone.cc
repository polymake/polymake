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

namespace polymake { namespace polytope { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   FunctionWrapper4perl( perl::Object (polymake::graph::HasseDiagram, pm::graph::Graph<pm::graph::Undirected>, pm::graph::EdgeMap<pm::graph::Undirected, pm::Vector<pm::Rational>, void>) ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]);
      IndirectWrapperReturn( arg0, arg1.get< perl::TryCanned< const Graph< Undirected > > >(), arg2.get< perl::TryCanned< const EdgeMap< Undirected, Vector< Rational > > > >() );
   }
   FunctionWrapperInstance4perl( perl::Object (polymake::graph::HasseDiagram, pm::graph::Graph<pm::graph::Undirected>, pm::graph::EdgeMap<pm::graph::Undirected, pm::Vector<pm::Rational>, void>) );

   FunctionWrapper4perl( perl::Object (pm::Vector<pm::Rational>, pm::Matrix<pm::Rational>, perl::Object) ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]);
      IndirectWrapperReturn( arg0.get< perl::TryCanned< const Vector< Rational > > >(), arg1.get< perl::TryCanned< const Matrix< Rational > > >(), arg2 );
   }
   FunctionWrapperInstance4perl( perl::Object (pm::Vector<pm::Rational>, pm::Matrix<pm::Rational>, perl::Object) );

   FunctionWrapper4perl( perl::Object (pm::Vector<pm::Rational>, perl::Object, perl::Object) ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]);
      IndirectWrapperReturn( arg0.get< perl::TryCanned< const Vector< Rational > > >(), arg1, arg2 );
   }
   FunctionWrapperInstance4perl( perl::Object (pm::Vector<pm::Rational>, perl::Object, perl::Object) );

   FunctionWrapper4perl( perl::Object (polymake::graph::HasseDiagram, pm::graph::Graph<pm::graph::Undirected>, pm::graph::EdgeMap<pm::graph::Undirected, pm::Vector<pm::Rational>, void>, pm::Set<int, pm::operations::cmp>) ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]), arg3(stack[3]);
      IndirectWrapperReturn( arg0, arg1.get< perl::TryCanned< const Graph< Undirected > > >(), arg2.get< perl::TryCanned< const EdgeMap< Undirected, Vector< Rational > > > >(), arg3.get< perl::TryCanned< const Set< int > > >() );
   }
   FunctionWrapperInstance4perl( perl::Object (polymake::graph::HasseDiagram, pm::graph::Graph<pm::graph::Undirected>, pm::graph::EdgeMap<pm::graph::Undirected, pm::Vector<pm::Rational>, void>, pm::Set<int, pm::operations::cmp>) );

   FunctionWrapper4perl( perl::Object (pm::Vector<pm::Rational>, pm::Matrix<pm::Rational>, perl::Object, pm::Set<int, pm::operations::cmp>) ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]), arg3(stack[3]);
      IndirectWrapperReturn( arg0.get< perl::TryCanned< const Vector< Rational > > >(), arg1.get< perl::TryCanned< const Matrix< Rational > > >(), arg2, arg3.get< perl::TryCanned< const Set< int > > >() );
   }
   FunctionWrapperInstance4perl( perl::Object (pm::Vector<pm::Rational>, pm::Matrix<pm::Rational>, perl::Object, pm::Set<int, pm::operations::cmp>) );

   FunctionWrapper4perl( perl::Object (pm::Vector<pm::Rational>, perl::Object, perl::Object, pm::Set<int, pm::operations::cmp>, pm::Matrix<pm::Rational>) ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]), arg3(stack[3]), arg4(stack[4]);
      IndirectWrapperReturn( arg0.get< perl::TryCanned< const Vector< Rational > > >(), arg1, arg2, arg3.get< perl::TryCanned< const Set< int > > >(), arg4.get< perl::TryCanned< const Matrix< Rational > > >() );
   }
   FunctionWrapperInstance4perl( perl::Object (pm::Vector<pm::Rational>, perl::Object, perl::Object, pm::Set<int, pm::operations::cmp>, pm::Matrix<pm::Rational>) );

///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
