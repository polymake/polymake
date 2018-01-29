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

///==== this line controls the automatic file splitting: max.instances=40

#include "polymake/Integer.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/client.h"
#include "polymake/linalg.h"
#include "polymake/numerical_functions.h"

namespace polymake { namespace common { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0>
   FunctionInterface4perl( lcm_X, T0 ) {
      perl::Value arg0(stack[0]);
      WrapperReturn( (lcm(arg0.get<T0>())) );
   };

   template <typename T0, typename T1>
   FunctionInterface4perl( lcm_X_X, T0,T1 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      WrapperReturn( lcm(arg0.get<T0>(), arg1.get<T1>()) );
   };

   FunctionInstance4perl(lcm_X_X, long, long);
   FunctionInstance4perl(lcm_X_X, perl::Canned< const Integer >, perl::Canned< const Integer >);
   FunctionInstance4perl(lcm_X, perl::Canned< const Vector< Integer > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
