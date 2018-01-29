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

namespace polymake { namespace tropical { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   FunctionWrapper4perl( pm::Array<pm::Set<int, pm::operations::cmp>> (int, pm::Array<pm::Set<int, pm::operations::cmp>> const&, pm::Array<int>) ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]);
      IndirectWrapperReturn( arg0, arg1.get< perl::TryCanned< const Array< Set< int > > > >(), arg2.get< perl::TryCanned< const Array< int > > >() );
   }
   FunctionWrapperInstance4perl( pm::Array<pm::Set<int, pm::operations::cmp>> (int, pm::Array<pm::Set<int, pm::operations::cmp>> const&, pm::Array<int>) );

   FunctionWrapper4perl( pm::IncidenceMatrix<pm::NonSymmetric> (int, pm::Array<pm::Set<int, pm::operations::cmp>> const&, pm::Array<int>) ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]);
      IndirectWrapperReturn( arg0, arg1.get< perl::TryCanned< const Array< Set< int > > > >(), arg2.get< perl::TryCanned< const Array< int > > >() );
   }
   FunctionWrapperInstance4perl( pm::IncidenceMatrix<pm::NonSymmetric> (int, pm::Array<pm::Set<int, pm::operations::cmp>> const&, pm::Array<int>) );

   FunctionWrapper4perl( pm::Map<pm::Set<int, pm::operations::cmp>, int, pm::operations::cmp> (pm::IncidenceMatrix<pm::NonSymmetric> const&) ) {
      perl::Value arg0(stack[0]);
      IndirectWrapperReturn( arg0.get< perl::TryCanned< const IncidenceMatrix< NonSymmetric > > >() );
   }
   FunctionWrapperInstance4perl( pm::Map<pm::Set<int, pm::operations::cmp>, int, pm::operations::cmp> (pm::IncidenceMatrix<pm::NonSymmetric> const&) );

   FunctionWrapper4perl( perl::Object (pm::IncidenceMatrix<pm::NonSymmetric> const&, int) ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      IndirectWrapperReturn( arg0.get< perl::TryCanned< const IncidenceMatrix< NonSymmetric > > >(), arg1 );
   }
   FunctionWrapperInstance4perl( perl::Object (pm::IncidenceMatrix<pm::NonSymmetric> const&, int) );

///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
