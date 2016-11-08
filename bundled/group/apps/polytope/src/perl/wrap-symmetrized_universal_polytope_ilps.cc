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

namespace polymake { namespace polytope { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   FunctionWrapper4perl( perl::Object (int, pm::Matrix<pm::Rational> const&, pm::Array<pm::boost_dynamic_bitset> const&, pm::Rational const&, pm::Array<pm::Array<int>> const&, pm::SparseMatrix<pm::Rational, pm::NonSymmetric> const&, perl::OptionSet) ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]), arg3(stack[3]), arg4(stack[4]), arg5(stack[5]), arg6(stack[6]);
      IndirectWrapperReturn( arg0, arg1.get< perl::TryCanned< const Matrix< Rational > > >(), arg2.get< perl::TryCanned< const Array< boost_dynamic_bitset > > >(), arg3.get< perl::TryCanned< const Rational > >(), arg4.get< perl::TryCanned< const Array< Array< int > > > >(), arg5.get< perl::TryCanned< const SparseMatrix< Rational, NonSymmetric > > >(), arg6 );
   }
   FunctionWrapperInstance4perl( perl::Object (int, pm::Matrix<pm::Rational> const&, pm::Array<pm::boost_dynamic_bitset> const&, pm::Rational const&, pm::Array<pm::Array<int>> const&, pm::SparseMatrix<pm::Rational, pm::NonSymmetric> const&, perl::OptionSet) );

///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
