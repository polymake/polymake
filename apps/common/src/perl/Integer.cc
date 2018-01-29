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

///==== this line controls the automatic file splitting: max.instances=60

#include "polymake/Integer.h"
#include "polymake/Rational.h"
#include "polymake/client.h"

namespace polymake { namespace common { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1>
   FunctionInterface4perl( new_X, T0,T1 ) {
      perl::Value arg0(stack[1]);
      WrapperReturnNew(T0, (arg0.get<T1>()) );
   };

   OperatorInstance4perl(Binary_add, perl::Canned< const pm::RationalParticle<true, pm::Integer> >, perl::Canned< const pm::RationalParticle<false, pm::Integer> >);
   OperatorInstance4perl(Binary_sub, perl::Canned< const pm::RationalParticle<true, pm::Integer> >, perl::Canned< const pm::RationalParticle<false, pm::Integer> >);
   FunctionInstance4perl(new_X, Integer, perl::Canned< const pm::RationalParticle<true, pm::Integer> >);
   FunctionInstance4perl(new_X, Integer, perl::Canned< const pm::RationalParticle<false, pm::Integer> >);
   OperatorInstance4perl(Binary_mod, perl::Canned< const Integer >, long);
   OperatorInstance4perl(Binary_mod, long, perl::Canned< const Integer >);
   OperatorInstance4perl(Binary__gt, int, perl::Canned< const Integer >);
   OperatorInstance4perl(BinaryAssign_add, perl::Canned< Integer >, long);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
