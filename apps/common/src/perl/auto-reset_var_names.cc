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

#include "polymake/Polynomial.h"
#include "polymake/Rational.h"
#include "polymake/RationalFunction.h"
#include "polymake/client.h"

namespace polymake { namespace common { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0>
   FunctionInterface4perl( UniPolynomial__reset_var_names_f17, T0 ) {
      WrapperReturnVoid( T0::reset_var_names() );
   };

   template <typename T0>
   FunctionInterface4perl( Polynomial__reset_var_names_f17, T0 ) {
      WrapperReturnVoid( T0::reset_var_names() );
   };

   FunctionInstance4perl(Polynomial__reset_var_names_f17, Polynomial< Rational, int >);
   FunctionInstance4perl(UniPolynomial__reset_var_names_f17, UniPolynomial< UniPolynomial< Rational, int >, Rational >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }