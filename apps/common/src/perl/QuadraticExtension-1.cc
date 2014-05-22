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
#include "polymake/QuadraticExtension.h"
#include "polymake/Rational.h"
#include "polymake/Integer.h"

namespace polymake { namespace common { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0>
   FunctionInterface4perl( new, T0 ) {
      WrapperReturnNew(T0, () );
   };

   Class4perl("Polymake::common::QuadraticExtension__Rational", QuadraticExtension< Rational >);
   ClassTemplate4perl("Polymake::common::QuadraticExtension");
   FunctionInstance4perl(new, QuadraticExtension< Rational >);
   OperatorInstance4perl(Unary_neg, perl::Canned< const QuadraticExtension< Rational > >);
   OperatorInstance4perl(Binary_add, perl::Canned< const QuadraticExtension< Rational > >, perl::Canned< const QuadraticExtension< Rational > >);
   OperatorInstance4perl(Binary_add, perl::Canned< const QuadraticExtension< Rational > >, perl::Canned< const Rational >);
   OperatorInstance4perl(Binary_add, perl::Canned< const QuadraticExtension< Rational > >, perl::Canned< const Integer >);
   OperatorInstance4perl(Binary_add, perl::Canned< const QuadraticExtension< Rational > >, int);
   OperatorInstance4perl(Binary_add, int, perl::Canned< const QuadraticExtension< Rational > >);
   OperatorInstance4perl(Binary_sub, perl::Canned< const QuadraticExtension< Rational > >, perl::Canned< const QuadraticExtension< Rational > >);
   OperatorInstance4perl(Binary_sub, perl::Canned< const QuadraticExtension< Rational > >, perl::Canned< const Rational >);
   OperatorInstance4perl(Binary_sub, perl::Canned< const QuadraticExtension< Rational > >, perl::Canned< const Integer >);
   OperatorInstance4perl(Binary_sub, perl::Canned< const QuadraticExtension< Rational > >, int);
   OperatorInstance4perl(Binary_sub, int, perl::Canned< const QuadraticExtension< Rational > >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const QuadraticExtension< Rational > >, perl::Canned< const QuadraticExtension< Rational > >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const QuadraticExtension< Rational > >, perl::Canned< const Rational >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const QuadraticExtension< Rational > >, perl::Canned< const Integer >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const QuadraticExtension< Rational > >, int);
   OperatorInstance4perl(Binary_mul, int, perl::Canned< const QuadraticExtension< Rational > >);
   OperatorInstance4perl(Binary_div, perl::Canned< const QuadraticExtension< Rational > >, perl::Canned< const QuadraticExtension< Rational > >);
   OperatorInstance4perl(Binary_div, perl::Canned< const QuadraticExtension< Rational > >, perl::Canned< const Rational >);
   OperatorInstance4perl(Binary_div, perl::Canned< const QuadraticExtension< Rational > >, perl::Canned< const Integer >);
   OperatorInstance4perl(Binary_div, perl::Canned< const QuadraticExtension< Rational > >, int);
   OperatorInstance4perl(Binary_div, int, perl::Canned< const QuadraticExtension< Rational > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const QuadraticExtension< Rational > >, perl::Canned< const QuadraticExtension< Rational > >);
   OperatorInstance4perl(Binary__ne, perl::Canned< const QuadraticExtension< Rational > >, perl::Canned< const QuadraticExtension< Rational > >);
   OperatorInstance4perl(Binary__lt, perl::Canned< const QuadraticExtension< Rational > >, perl::Canned< const QuadraticExtension< Rational > >);
   OperatorInstance4perl(Binary__gt, perl::Canned< const QuadraticExtension< Rational > >, perl::Canned< const QuadraticExtension< Rational > >);
   OperatorInstance4perl(Binary__le, perl::Canned< const QuadraticExtension< Rational > >, perl::Canned< const QuadraticExtension< Rational > >);
   OperatorInstance4perl(Binary__ge, perl::Canned< const QuadraticExtension< Rational > >, perl::Canned< const QuadraticExtension< Rational > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const QuadraticExtension< Rational > >, perl::Canned< const Rational >);
   OperatorInstance4perl(Binary__ne, perl::Canned< const QuadraticExtension< Rational > >, perl::Canned< const Rational >);
   OperatorInstance4perl(Binary__lt, perl::Canned< const QuadraticExtension< Rational > >, perl::Canned< const Rational >);
   OperatorInstance4perl(Binary__gt, perl::Canned< const QuadraticExtension< Rational > >, perl::Canned< const Rational >);
   OperatorInstance4perl(Binary__le, perl::Canned< const QuadraticExtension< Rational > >, perl::Canned< const Rational >);
   OperatorInstance4perl(Binary__ge, perl::Canned< const QuadraticExtension< Rational > >, perl::Canned< const Rational >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const QuadraticExtension< Rational > >, perl::Canned< const Integer >);
   OperatorInstance4perl(Binary__ne, perl::Canned< const QuadraticExtension< Rational > >, perl::Canned< const Integer >);
   OperatorInstance4perl(Binary__lt, perl::Canned< const QuadraticExtension< Rational > >, perl::Canned< const Integer >);
   OperatorInstance4perl(Binary__gt, perl::Canned< const QuadraticExtension< Rational > >, perl::Canned< const Integer >);
   OperatorInstance4perl(Binary__le, perl::Canned< const QuadraticExtension< Rational > >, perl::Canned< const Integer >);
   OperatorInstance4perl(Binary__ge, perl::Canned< const QuadraticExtension< Rational > >, perl::Canned< const Integer >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const QuadraticExtension< Rational > >, int);
   OperatorInstance4perl(Binary__ne, perl::Canned< const QuadraticExtension< Rational > >, int);
   OperatorInstance4perl(Binary__lt, perl::Canned< const QuadraticExtension< Rational > >, int);
   OperatorInstance4perl(Binary__gt, perl::Canned< const QuadraticExtension< Rational > >, int);
   OperatorInstance4perl(Binary__le, perl::Canned< const QuadraticExtension< Rational > >, int);
   OperatorInstance4perl(Binary__ge, perl::Canned< const QuadraticExtension< Rational > >, int);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
