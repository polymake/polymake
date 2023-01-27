/* Copyright (c) 1997-2023
   Ewgenij Gawrilow, Michael Joswig, and the polymake team
   Technische Universität Berlin, Germany
   https://polymake.org

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

#pragma once

#include "polymake/Map.h"
#include "polymake/Integer.h"
#include "polymake/Rational.h"
#include "polymake/TropicalNumber.h"

namespace pm {
namespace flint {

Map<Integer, Int> factor(const Integer&);
Integer expand(const Map<Integer, Int>&);
TropicalNumber<Min> valuation(const Rational&, const Integer&);
std::pair<Integer, Integer> factor_out_squares(const Integer&);

}
} // pm

