/* Copyright (c) 1997-2022
   Ewgenij Gawrilow, Michael Joswig, and the polymake team
   Technische Universität Berlin, Germany
   https://polymake.org

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version: http://www.gnu.org/licenses/gpl.txt.

   This program is distributed in the hope that it will be useful,
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
--------------------------------------------------------------------------------
*/

#pragma once
#include "polymake/Rational.h"
#include "polymake/QuadraticExtension.h"
#include "TOSimplex/TOmath.h"

template<>
inline pm::Rational
TOmath<pm::Rational>::hugeposint() {
   return pm::Rational( std::numeric_limits<long>::max() );
}

template<>
inline pm::Rational
TOmath<pm::Rational>::floor( const pm::Rational &a ) {
   return a.floor();
}

template<>
inline pm::Rational
TOmath<pm::Rational>::ceil( const pm::Rational &a ) {
   return a.ceil();
}

template<>
inline pm::QuadraticExtension<pm::Rational>
TOmath<pm::QuadraticExtension<pm::Rational>>::hugeposint() {
   return pm::QuadraticExtension<pm::Rational>( std::numeric_limits<long>::max() );
}

template<>
inline pm::QuadraticExtension<pm::Rational>
TOmath<pm::QuadraticExtension<pm::Rational>>::floor( const pm::QuadraticExtension<pm::Rational> &a ) {
   return pm::QuadraticExtension<pm::Rational>( a.to_field_type().floor() );
}

template<>
inline pm::QuadraticExtension<pm::Rational>
TOmath<pm::QuadraticExtension<pm::Rational>>::ceil( const pm::QuadraticExtension<pm::Rational> &a ) {
   return pm::QuadraticExtension<pm::Rational>( a.to_field_type().ceil() );
}

template <>
inline std::string
TOmath<pm::QuadraticExtension<pm::Rational>>::toShortString( const pm::QuadraticExtension<pm::Rational> &a ){
   std::stringstream str;
   str << "( (" << a.a() << ") + (" << a.b() << ") r " << a.r() << ")"; 
   return str.str();
}


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
