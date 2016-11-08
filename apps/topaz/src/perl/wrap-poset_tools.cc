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

namespace polymake { namespace topaz { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   FunctionWrapper4perl( pm::Array<pm::Array<int>> (pm::graph::Graph<pm::graph::Directed> const&, pm::graph::Graph<pm::graph::Directed> const&) ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      IndirectWrapperReturn( arg0.get< perl::TryCanned< const Graph< Directed > > >(), arg1.get< perl::TryCanned< const Graph< Directed > > >() );
   }
   FunctionWrapperInstance4perl( pm::Array<pm::Array<int>> (pm::graph::Graph<pm::graph::Directed> const&, pm::graph::Graph<pm::graph::Directed> const&) );

   FunctionWrapper4perl( pm::Array<pm::Array<int>> (perl::Object, perl::Object) ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      IndirectWrapperReturn( arg0, arg1 );
   }
   FunctionWrapperInstance4perl( pm::Array<pm::Array<int>> (perl::Object, perl::Object) );

   FunctionWrapper4perl( pm::Set<pm::Array<int>, pm::operations::cmp> (perl::Object, perl::Object) ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      IndirectWrapperReturn( arg0, arg1 );
   }
   FunctionWrapperInstance4perl( pm::Set<pm::Array<int>, pm::operations::cmp> (perl::Object, perl::Object) );

   FunctionWrapper4perl( pm::Set<pm::Array<int>, pm::operations::cmp> (perl::Object, perl::Object, perl::OptionSet) ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]);
      IndirectWrapperReturn( arg0, arg1, arg2 );
   }
   FunctionWrapperInstance4perl( pm::Set<pm::Array<int>, pm::operations::cmp> (perl::Object, perl::Object, perl::OptionSet) );

   FunctionWrapper4perl( pm::graph::Graph<pm::graph::Directed> (perl::Object, perl::Object) ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      IndirectWrapperReturn( arg0, arg1 );
   }
   FunctionWrapperInstance4perl( pm::graph::Graph<pm::graph::Directed> (perl::Object, perl::Object) );

   FunctionWrapper4perl( pm::graph::Graph<pm::graph::Directed> (perl::Object) ) {
      perl::Value arg0(stack[0]);
      IndirectWrapperReturn( arg0 );
   }
   FunctionWrapperInstance4perl( pm::graph::Graph<pm::graph::Directed> (perl::Object) );

   FunctionWrapper4perl( pm::graph::Graph<pm::graph::Directed> (pm::Set<pm::Array<int>, pm::operations::cmp> const&, perl::Object) ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      IndirectWrapperReturn( arg0.get< perl::TryCanned< const Set< Array< int > > > >(), arg1 );
   }
   FunctionWrapperInstance4perl( pm::graph::Graph<pm::graph::Directed> (pm::Set<pm::Array<int>, pm::operations::cmp> const&, perl::Object) );

///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
