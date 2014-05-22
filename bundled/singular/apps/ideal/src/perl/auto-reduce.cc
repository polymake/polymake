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

///==== this line controls the automatic file splitting: max.instances=40

#include "polymake/client.h"
#include "polymake/ideal/singularIdeal.h"
#include "polymake/Polynomial.h"
#include "polymake/Rational.h"
#include "polymake/Ring.h"

namespace polymake { namespace ideal { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1, typename T2>
   FunctionInterface4perl( reduce_X_X_f1, T0,T1,T2 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]);
      WrapperReturn( arg0.get<T0>().reduce(arg1.get<T1>(), arg2.get<T2>()) );
   };

   FunctionInstance4perl(reduce_X_X_f1, perl::Canned< const SingularIdeal >, perl::Canned< const Polynomial< Rational, int > >, perl::Canned< const Ring< Rational, int > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
