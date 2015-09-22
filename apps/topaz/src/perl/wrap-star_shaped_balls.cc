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

#include "polymake/Array.h"
#include "polymake/Rational.h"
#include "polymake/Set.h"

namespace polymake { namespace topaz { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1>
   FunctionInterface4perl( poset_by_inclusion_T_X, T0,T1 ) {
      perl::Value arg0(stack[0]);
      WrapperReturn( (poset_by_inclusion<T0>(arg0.get<T1>())) );
   };

   template <typename T0>
   FunctionInterface4perl( star_shaped_balls_T_x, T0 ) {
      perl::Value arg0(stack[0]);
      WrapperReturn( (star_shaped_balls<T0>(arg0)) );
   };

   template <typename T0>
   FunctionInterface4perl( star_of_zero_T_x, T0 ) {
      perl::Value arg0(stack[0]);
      WrapperReturn( (star_of_zero<T0>(arg0)) );
   };

   FunctionInstance4perl(star_shaped_balls_T_x, Rational);
   FunctionInstance4perl(star_of_zero_T_x, Rational);
   FunctionInstance4perl(poset_by_inclusion_T_X, Set< Set< int > >, perl::Canned< const Array< Set< Set< int > > > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
