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

#include "polymake/client.h"
#include "polymake/tropical/dual_addition_version.h"

namespace polymake { namespace tropical {
	
	UserFunctionTemplate4perl("# @category Conversion of tropical addition"
			"# This function takes a tropical number and returns a tropical number that "
			"# uses the opposite tropical addition. By default, the sign is inverted."
			"# @param TropicalNumber<Addition> number "
			"# @param Bool strong_conversion This is optional and TRUE by default."
			"# It indicates, whether the sign of the number should be inverted."
			"# @return TropicalNumber",
			"dual_addition_version<Addition>(TropicalNumber<Addition>;$=1)");

	UserFunctionTemplate4perl("# @category Conversion of tropical addition"
			"# This function takes a vector of tropical numbers and returns a vector that "
			"# uses the opposite tropical addition. By default, the signs of the entries are inverted."
			"# @param Vector<TropicalNumber<Addition> > vector"
			"# @param Bool strong_conversion This is optional and TRUE by default."
			"# It indicates, whether the signs of the entries should be inverted."
			"# @return Vector<TropicalNumber>",
			"dual_addition_version<Addition>(Vector<TropicalNumber<Addition> >;$=1)");

	UserFunctionTemplate4perl("# @category Conversion of tropical addition"
			"# This function takes a matrix of tropical numbers and returns a matrix that "
			"# uses the opposite tropical addition. By default, the signs of the entries are inverted."
			"# @param Matrix<TropicalNumber<Addition> > matrix "
			"# @param Bool strong_conversion This is optional and TRUE by default."
			"# It indicates, whether the signs of the entries should be inverted."
			"# @return Matrix<TropicalNumber>",
			"dual_addition_version<Addition>(Matrix<TropicalNumber<Addition> >;$=1)");

	UserFunctionTemplate4perl("# @category Conversion of tropical addition"
			"# This function takes a ring over the tropical numbers and returns a ring that"
			"# uses the opposite tropical addition. Variable names are preserved"
			"# @param Ring<TropicalNumber<Addition> > ring"
			"# @return Ring<TropicalNumber>",
			"dual_addition_version<Addition>(Ring<TropicalNumber<Addition> >)");

	UserFunctionTemplate4perl("# @category Conversion of tropical addition"
			"# This function takes a tropical polynomial and returns a tropical polynomial that "
			"# uses the opposite tropical addition. By default, the signs of the coefficients are inverted."
			"# @param Polynomial<TropicalNumber<Addition> > polynomial "
			"# @param Bool strong_conversion This is optional and TRUE by default."
			"# It indicates, whether the signs of the coefficients should be inverted."
			"# @return Polynomial<TropicalNumber>",
			"dual_addition_version<Addition>(Polynomial<TropicalNumber<Addition> >; $=1)");




}}
