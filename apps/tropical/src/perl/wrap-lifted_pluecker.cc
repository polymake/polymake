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

#include "polymake/IncidenceMatrix.h"
#include "polymake/Matrix.h"
#include "polymake/Rational.h"
#include "polymake/TropicalNumber.h"
#include "polymake/tropical/arithmetic.h"

namespace polymake { namespace tropical { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1>
   FunctionInterface4perl( lifted_pluecker_T_X, T0,T1 ) {
      perl::Value arg0(stack[0]);
      WrapperReturn( (lifted_pluecker<T0>(arg0.get<T1>())) );
   };

   FunctionInstance4perl(lifted_pluecker_T_X, Min, perl::Canned< const Matrix< TropicalNumber< Min, Rational > > >);
   FunctionInstance4perl(lifted_pluecker_T_X, Max, perl::Canned< const Matrix< TropicalNumber< Max, Rational > > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
