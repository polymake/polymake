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

#include "polymake/Integer.h"
#include "polymake/Matrix.h"
#include "polymake/Rational.h"
#include "polymake/Vector.h"

namespace polymake { namespace polytope { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0>
   FunctionInterface4perl( binomial_to_power_basis_X, T0 ) {
      perl::Value arg0(stack[0]);
      WrapperReturn( binomial_to_power_basis(arg0.get<T0>()) );
   };

   template <typename T0>
   FunctionInterface4perl( power_to_binomial_basis_X, T0 ) {
      perl::Value arg0(stack[0]);
      WrapperReturn( power_to_binomial_basis(arg0.get<T0>()) );
   };

   FunctionInstance4perl(binomial_to_power_basis_X, perl::Canned< const Vector< Rational > >);
   FunctionInstance4perl(power_to_binomial_basis_X, perl::Canned< const Vector< Rational > >);
   FunctionInstance4perl(binomial_to_power_basis_X, perl::Canned< const pm::IndexedSlice<pm::Vector<pm::Integer> const&, pm::Series<int, true>, void> >);
   FunctionInstance4perl(power_to_binomial_basis_X, perl::Canned< const pm::IndexedSlice<pm::Vector<pm::Rational> const&, pm::Series<int, true>, void> >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
