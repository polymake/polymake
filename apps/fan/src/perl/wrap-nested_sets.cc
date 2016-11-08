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

namespace polymake { namespace fan { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   FunctionWrapper4perl( pm::PowerSet<int, pm::operations::cmp> (pm::Array<pm::Set<int, pm::operations::cmp>> const&, int) ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      IndirectWrapperReturn( arg0.get< perl::TryCanned< const Array< Set< int > > > >(), arg1 );
   }
   FunctionWrapperInstance4perl( pm::PowerSet<int, pm::operations::cmp> (pm::Array<pm::Set<int, pm::operations::cmp>> const&, int) );

   FunctionWrapper4perl( bool (pm::Set<pm::Set<int, pm::operations::cmp>, pm::operations::cmp> const&, pm::PowerSet<int, pm::operations::cmp> const&) ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      IndirectWrapperReturn( arg0.get< perl::TryCanned< const Set< Set< int > > > >(), arg1.get< perl::TryCanned< const PowerSet< int > > >() );
   }
   FunctionWrapperInstance4perl( bool (pm::Set<pm::Set<int, pm::operations::cmp>, pm::operations::cmp> const&, pm::PowerSet<int, pm::operations::cmp> const&) );

   FunctionWrapper4perl( bool (pm::PowerSet<int, pm::operations::cmp> const&, int) ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      IndirectWrapperReturn( arg0.get< perl::TryCanned< const PowerSet< int > > >(), arg1 );
   }
   FunctionWrapperInstance4perl( bool (pm::PowerSet<int, pm::operations::cmp> const&, int) );

///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
