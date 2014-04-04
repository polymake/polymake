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

namespace polymake { namespace topaz { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   FunctionWrapper4perl( pm::Array<pm::PowerSet<int, pm::operations::cmp>, void> (perl::Object, perl::OptionSet) ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      IndirectWrapperReturn( arg0, arg1 );
   }
   FunctionWrapperInstance4perl( pm::Array<pm::PowerSet<int, pm::operations::cmp>, void> (perl::Object, perl::OptionSet) );

   FunctionWrapper4perl( pm::Array<pm::PowerSet<int, pm::operations::cmp>, void> (pm::Array<pm::Set<int, pm::operations::cmp>, void> const&, perl::OptionSet) ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      IndirectWrapperReturn( arg0.get< perl::TryCanned< const Array< Set< int > > > >(), arg1 );
   }
   FunctionWrapperInstance4perl( pm::Array<pm::PowerSet<int, pm::operations::cmp>, void> (pm::Array<pm::Set<int, pm::operations::cmp>, void> const&, perl::OptionSet) );

///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
