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

namespace polymake { namespace graph { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   FunctionWrapper4perl( polymake::graph::SpringEmbedderWindow* (pm::graph::Graph<pm::graph::Undirected> const&, perl::OptionSet) ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      IndirectWrapperReturn(arg0.get< perl::TryCanned< const Graph< Undirected > > >(), arg1);
   }
   FunctionWrapperInstance4perl( polymake::graph::SpringEmbedderWindow* (pm::graph::Graph<pm::graph::Undirected> const&, perl::OptionSet) );

   template <typename T0>
   FunctionInterface4perl( port_f1, T0 ) {
      perl::Value arg0(stack[0]);
      WrapperReturn( arg0.get<T0>().port() );
   };

   FunctionInstance4perl(port_f1, perl::Canned< const SpringEmbedderWindow >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
