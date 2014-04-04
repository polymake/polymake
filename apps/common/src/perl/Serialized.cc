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
#include "polymake/Polynomial.h"
#include "polymake/Rational.h"
#include "polymake/RationalFunction.h"
#include "polymake/Ring.h"
#include "polymake/QuadraticExtension.h"

namespace polymake { namespace common { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   ClassTemplate4perl("Polymake::common::Serialized");
   Class4perl("Polymake::common::Serialized__QuadraticExtension__Rational", pm::Serialized< QuadraticExtension< Rational > >);
   Class4perl("Polymake::common::Serialized__UniPolynomial_A_Rational_I_Int_Z", pm::Serialized< UniPolynomial< Rational, int > >);
   Class4perl("Polymake::common::Serialized__Term_A_Rational_I_Int_Z", pm::Serialized< Term< Rational, int > >);
   Class4perl("Polymake::common::Serialized__RationalFunction_A_Rational_I_Int_Z", pm::Serialized< RationalFunction< Rational, int > >);
   Class4perl("Polymake::common::Serialized__Ring_A_Rational_I_Int_Z", pm::Serialized< Ring< Rational, int > >);
   Class4perl("Polymake::common::Serialized__Polynomial_A_Rational_I_Int_Z", pm::Serialized< Polynomial< Rational, int > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
