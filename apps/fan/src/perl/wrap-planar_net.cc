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

#include "polymake/QuadraticExtension.h"
#include "polymake/Rational.h"

namespace polymake { namespace fan { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0>
   FunctionInterface4perl( planar_net_T_x, T0 ) {
      perl::Value arg0(stack[0]);
      WrapperReturn( (planar_net<T0>(arg0)) );
   };

   FunctionInstance4perl(planar_net_T_x, Rational);
   FunctionInstance4perl(planar_net_T_x, QuadraticExtension< Rational >);
   FunctionInstance4perl(planar_net_T_x, double);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
