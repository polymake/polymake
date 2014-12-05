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

#include "polymake/Rational.h"
#include "polymake/Vector.h"

namespace polymake { namespace polytope { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0>
   FunctionInterface4perl( cell_from_subdivision_T_x_x_o, T0 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]);
      WrapperReturn( (cell_from_subdivision<T0>(arg0, arg1, arg2)) );
   };

   template <typename T0>
   FunctionInterface4perl( cells_from_subdivision_T_x_x_o, T0 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]);
      WrapperReturn( (cells_from_subdivision<T0>(arg0, arg1, arg2)) );
   };

   FunctionWrapper4perl( perl::Object (perl::Object, pm::Set<int, pm::operations::cmp> const&, perl::OptionSet) ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]);
      IndirectWrapperReturn(arg0, arg1.get< perl::TryCanned< const Set< int > > >(), arg2);
   }
   FunctionWrapperInstance4perl( perl::Object (perl::Object, pm::Set<int, pm::operations::cmp> const&, perl::OptionSet) );

   OperatorInstance4perl(Binary_diva, perl::Canned< const Wary< pm::MatrixMinor<pm::Matrix<pm::Rational> const&, pm::Set<int, pm::operations::cmp> const&, pm::all_selector const&> > >, perl::Canned< const Vector< Rational > >);
   OperatorInstance4perl(Binary_diva, perl::Canned< const Wary< pm::RowChain<pm::MatrixMinor<pm::Matrix<pm::Rational> const&, pm::Set<int, pm::operations::cmp> const&, pm::all_selector const&> const&, pm::SingleRow<pm::Vector<pm::Rational> const&> > > >, perl::Canned< const Vector< Rational > >);
   FunctionInstance4perl(cell_from_subdivision_T_x_x_o, Rational);
   FunctionInstance4perl(cells_from_subdivision_T_x_x_o, Rational);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
