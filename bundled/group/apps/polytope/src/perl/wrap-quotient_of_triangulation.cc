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
#include "polymake/Set.h"
#include "polymake/common/boost_dynamic_bitset.h"

namespace polymake { namespace polytope { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1, typename T2, typename T3, typename T4>
   FunctionInterface4perl( quotient_of_triangulation_T_X_X_X_o, T0,T1,T2,T3,T4 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]), arg3(stack[3]);
      WrapperReturn( (quotient_of_triangulation<T0,T1>(arg0.get<T2>(), arg1.get<T3>(), arg2.get<T4>(), arg3)) );
   };

   FunctionInstance4perl(quotient_of_triangulation_T_X_X_X_o, boost_dynamic_bitset, Set< int >, perl::Canned< const Array< Set< int > > >, perl::Canned< const Array< Array< int > > >, perl::Canned< const Array< boost_dynamic_bitset > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
