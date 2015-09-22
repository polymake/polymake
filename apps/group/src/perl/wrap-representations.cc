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

namespace polymake { namespace group { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   FunctionWrapper4perl( pm::Array<int, void> (pm::Array<int, void> const&, perl::Object) ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      IndirectWrapperReturn( arg0.get< perl::TryCanned< const Array< int > > >(), arg1 );
   }
   FunctionWrapperInstance4perl( pm::Array<int, void> (pm::Array<int, void> const&, perl::Object) );

   FunctionWrapper4perl( pm::SparseMatrix<pm::Rational, pm::NonSymmetric> (perl::Object, int) ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      IndirectWrapperReturn( arg0, arg1 );
   }
   FunctionWrapperInstance4perl( pm::SparseMatrix<pm::Rational, pm::NonSymmetric> (perl::Object, int) );

   FunctionWrapper4perl( pm::SparseMatrix<pm::Rational, pm::NonSymmetric> (perl::Object, pm::Array<int, void> const&) ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      IndirectWrapperReturn( arg0, arg1.get< perl::TryCanned< const Array< int > > >() );
   }
   FunctionWrapperInstance4perl( pm::SparseMatrix<pm::Rational, pm::NonSymmetric> (perl::Object, pm::Array<int, void> const&) );

   FunctionWrapper4perl( pm::IncidenceMatrix<pm::NonSymmetric> (perl::Object, pm::Array<pm::Set<int, pm::operations::cmp>, void> const&) ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      IndirectWrapperReturn( arg0, arg1.get< perl::TryCanned< const Array< Set< int > > > >() );
   }
   FunctionWrapperInstance4perl( pm::IncidenceMatrix<pm::NonSymmetric> (perl::Object, pm::Array<pm::Set<int, pm::operations::cmp>, void> const&) );

   FunctionWrapper4perl( pm::IncidenceMatrix<pm::NonSymmetric> (perl::Object, pm::SparseMatrix<pm::Rational, pm::NonSymmetric> const&) ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      IndirectWrapperReturn( arg0, arg1.get< perl::TryCanned< const SparseMatrix< Rational, NonSymmetric > > >() );
   }
   FunctionWrapperInstance4perl( pm::IncidenceMatrix<pm::NonSymmetric> (perl::Object, pm::SparseMatrix<pm::Rational, pm::NonSymmetric> const&) );

   FunctionWrapper4perl( pm::Array<int, void> (pm::SparseMatrix<pm::Rational, pm::NonSymmetric> const&) ) {
      perl::Value arg0(stack[0]);
      IndirectWrapperReturn( arg0.get< perl::TryCanned< const SparseMatrix< Rational, NonSymmetric > > >() );
   }
   FunctionWrapperInstance4perl( pm::Array<int, void> (pm::SparseMatrix<pm::Rational, pm::NonSymmetric> const&) );

///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
