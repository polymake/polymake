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

#include "polymake/Matrix.h"
#include "polymake/Rational.h"

namespace polymake { namespace polytope { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0>
   FunctionInterface4perl( points2metric_max_X, T0 ) {
      perl::Value arg0(stack[0]);
      WrapperReturn( points2metric_max(arg0.get<T0>()) );
   };

   template <typename T0>
   FunctionInterface4perl( points2metric_l1_X, T0 ) {
      perl::Value arg0(stack[0]);
      WrapperReturn( points2metric_l1(arg0.get<T0>()) );
   };

   FunctionWrapper4perl( pm::Matrix<double> (pm::Matrix<double> const&) ) {
      perl::Value arg0(stack[0]);
      IndirectWrapperReturn( arg0.get< perl::TryCanned< const Matrix< double > > >() );
   }
   FunctionWrapperInstance4perl( pm::Matrix<double> (pm::Matrix<double> const&) );

   FunctionInstance4perl(points2metric_max_X, perl::Canned< const Matrix< Rational > >);
   FunctionInstance4perl(points2metric_l1_X, perl::Canned< const Matrix< Rational > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
