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

#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Array.h"
#include "polymake/Set.h"

namespace polymake { namespace polytope { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1, typename T2>
   FunctionInterface4perl( secondary_cone_T_X_X_o, T0,T1,T2 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]);
      WrapperReturn( (secondary_cone<T0>(arg0.get<T1>(), arg1.get<T2>(), arg2)) );
   };

   template <typename T0, typename T1>
   FunctionInterface4perl( regularity_lp_T_X_x_o, T0,T1 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]);
      WrapperReturn( (regularity_lp<T0>(arg0.get<T1>(), arg1, arg2)) );
   };

   template <typename T0, typename T1>
   FunctionInterface4perl( is_regular_T_X_x_o, T0,T1 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]);
      WrapperReturn( (is_regular<T0>(arg0.get<T1>(), arg1, arg2)) );
   };

   FunctionWrapper4perl( std::pair<bool, pm::Vector<pm::Rational> > (pm::Matrix<pm::Rational> const&, pm::Array<pm::Set<int, pm::operations::cmp>, void> const&, perl::OptionSet) ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]);
      IndirectWrapperReturn( arg0.get< perl::TryCanned< const Matrix< Rational > > >(), arg1.get< perl::TryCanned< const Array< Set< int > > > >(), arg2 );
   }
   FunctionWrapperInstance4perl( std::pair<bool, pm::Vector<pm::Rational> > (pm::Matrix<pm::Rational> const&, pm::Array<pm::Set<int, pm::operations::cmp>, void> const&, perl::OptionSet) );

   FunctionWrapper4perl( perl::Object (pm::Matrix<pm::Rational> const&, pm::Array<pm::Set<int, pm::operations::cmp>, void> const&, perl::OptionSet) ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]);
      IndirectWrapperReturn( arg0.get< perl::TryCanned< const Matrix< Rational > > >(), arg1.get< perl::TryCanned< const Array< Set< int > > > >(), arg2 );
   }
   FunctionWrapperInstance4perl( perl::Object (pm::Matrix<pm::Rational> const&, pm::Array<pm::Set<int, pm::operations::cmp>, void> const&, perl::OptionSet) );

   FunctionInstance4perl(secondary_cone_T_X_X_o, Rational, perl::Canned< const Matrix< Rational > >, perl::Canned< const Array< Set< int > > >);
   FunctionInstance4perl(regularity_lp_T_X_x_o, Rational, perl::Canned< const Matrix< Rational > >);
   FunctionInstance4perl(is_regular_T_X_x_o, Rational, perl::Canned< const Matrix< Rational > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
