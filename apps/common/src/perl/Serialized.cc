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

///==== this line controls the automatic file splitting: max.instances=40

#include "polymake/Bitset.h"
#include "polymake/Polynomial.h"
#include "polymake/PuiseuxFraction.h"
#include "polymake/QuadraticExtension.h"
#include "polymake/Rational.h"
#include "polymake/RationalFunction.h"
#include "polymake/TropicalNumber.h"
#include "polymake/client.h"

namespace polymake { namespace common { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   ClassTemplate4perl("Polymake::common::Serialized");
   Class4perl("Polymake::common::Serialized__QuadraticExtension__Rational", pm::Serialized< QuadraticExtension< Rational > >);
   Class4perl("Polymake::common::Serialized__Polynomial_A_Rational_I_Int_Z", pm::Serialized< Polynomial< Rational, int > >);
   Class4perl("Polymake::common::Serialized__RationalFunction_A_Rational_I_Int_Z", pm::Serialized< RationalFunction< Rational, int > >);
   Class4perl("Polymake::common::Serialized__PuiseuxFraction_A_Min_I_Rational_I_Rational_Z", pm::Serialized< PuiseuxFraction< Min, Rational, Rational > >);
   Class4perl("Polymake::common::Serialized__RationalFunction_A_Rational_I_Rational_Z", pm::Serialized< RationalFunction< Rational, Rational > >);
   Class4perl("Polymake::common::Serialized__PuiseuxFraction_A_Max_I_Rational_I_Rational_Z", pm::Serialized< PuiseuxFraction< Max, Rational, Rational > >);
   Class4perl("Polymake::common::Serialized__Polynomial_A_TropicalNumber_A_Max_I_Rational_Z_I_Int_Z", pm::Serialized< Polynomial< TropicalNumber< Max, Rational >, int > >);
   Class4perl("Polymake::common::Serialized__Polynomial_A_TropicalNumber_A_Min_I_Rational_Z_I_Int_Z", pm::Serialized< Polynomial< TropicalNumber< Min, Rational >, int > >);
   Class4perl("Polymake::common::Serialized__Polynomial_A_PuiseuxFraction_A_Min_I_Rational_I_Rational_Z_I_Int_Z", pm::Serialized< Polynomial< PuiseuxFraction< Min, Rational, Rational >, int > >);
   Class4perl("Polymake::common::Serialized__PuiseuxFraction_A_Min_I_PuiseuxFraction_A_Min_I_Rational_I_Rational_Z_I_Rational_Z", pm::Serialized< PuiseuxFraction< Min, PuiseuxFraction< Min, Rational, Rational >, Rational > >);
   Class4perl("Polymake::common::Serialized__RationalFunction_A_PuiseuxFraction_A_Min_I_Rational_I_Rational_Z_I_Rational_Z", pm::Serialized< RationalFunction< PuiseuxFraction< Min, Rational, Rational >, Rational > >);
   Class4perl("Polymake::common::Serialized__UniPolynomial_A_Rational_I_Int_Z", pm::Serialized< UniPolynomial< Rational, int > >);
   Class4perl("Polymake::common::Serialized__UniPolynomial_A_UniPolynomial_A_Rational_I_Int_Z_I_Rational_Z", pm::Serialized< UniPolynomial< UniPolynomial< Rational, int >, Rational > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
