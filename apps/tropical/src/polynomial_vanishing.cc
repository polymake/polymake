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

#include "polymake/client.h"
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/Set.h"
#include "polymake/tropical/polynomial_vanishing.h"

namespace polymake { namespace tropical {

    UserFunctionTemplate4perl(
      "# @category Tropical operations"
      "# Get the set of indices of a (tropical) polynomial `p` where the Min/Max is"
      "# attained when evaluating at a given point `pt`."
      "# @param Polynomial<TropicalNumber<Addition, Scalar>> p"
      "# @param Vector<TropicalNumber<Addition, Scalar>> pt"
      "# @tparam Addition Choose Min or Max"
      "# @tparam Scalar"
      "# @return Set<Int>",
      "polynomial_support<Addition, Scalar>(Polynomial<TropicalNumber<Addition, Scalar>>, Vector<TropicalNumber<Addition, Scalar>>)");

    UserFunctionTemplate4perl(
      "# @category Tropical operations"
      "# Check whether a tropical polynomial `p` vanishes at a point `pt`, i.e."
      "# attains its Min/Max twice."
      "# @param Polynomial<TropicalNumber<Addition, Scalar>> p"
      "# @param Vector<TropicalNumber<Addition, Scalar>> pt"
      "# @tparam Addition Choose Min or Max"
      "# @tparam Scalar"
      "# @return Bool",
      "polynomial_vanishes<Addition, Scalar>(Polynomial<TropicalNumber<Addition, Scalar>>, Vector<TropicalNumber<Addition, Scalar>>)");

}}
