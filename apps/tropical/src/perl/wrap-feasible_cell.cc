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

#include "polymake/Array.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Matrix.h"
#include "polymake/Rational.h"
#include "polymake/TropicalNumber.h"
#include "polymake/Vector.h"

namespace polymake { namespace tropical { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1, typename T2, typename T3>
   FunctionInterface4perl( trop_witness_T_X_X, T0,T1,T2,T3 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      WrapperReturn( (trop_witness<T0,T1>(arg0.get<T2>(), arg1.get<T3>())) );
   };

   FunctionInstance4perl(trop_witness_T_X_X, Min, Rational, perl::Canned< const Matrix< TropicalNumber< Min, Rational > > >, perl::Canned< const Array< int > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
