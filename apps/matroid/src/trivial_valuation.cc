/* Copyright (c) 1997-2020
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
#include "polymake/list"
#include "polymake/Vector.h"
#include "polymake/Matrix.h"
#include "polymake/Array.h"
#include "polymake/Set.h"
#include "polymake/TropicalNumber.h"



namespace polymake { namespace matroid {

template <typename Addition,typename Scalar>
BigObject trivial_valuation(BigObject matroid)
{
  Int n_elements = matroid.give("N_ELEMENTS");
  BigObject result("ValuatedMatroid", mlist<Addition, Scalar>());
  result.take("N_ELEMENTS") << n_elements;
  if (matroid.exists("CIRCUITS")) {
    Array<Set<Int>> circuits = matroid.give("CIRCUITS");
    Matrix<TropicalNumber<Addition, Scalar>> cval(circuits.size(), n_elements);
    Int index = 0;
    for (auto circ = entire(circuits); !circ.at_end(); ++circ, ++index) {
      cval.row(index).slice(*circ) = ones_vector<TropicalNumber<Addition, Scalar>>( (*circ).size());
    }
    result.take("CIRCUITS") << circuits;
    result.take("VALUATION_ON_CIRCUITS") << cval;
  }

  Array<Set<Int>> bases = matroid.give("BASES");
  Vector<TropicalNumber<Addition, Scalar>> bval = ones_vector<TropicalNumber<Addition, Scalar>>(bases.size());
  result.take("BASES") << bases;
  result.take("VALUATION_ON_BASES") << bval;

  return result;
}

UserFunctionTemplate4perl("# @category Producing a matroid from matroids"
                          "# This function takes a matroid and gives it the trivial valuation "
                          "# to produce a valuated matroid"
                          "# @param Matroid M A matroid"
                          "# @tparam Addition The tropical addition to use, i.e. Min or Max"
                          "# @tparam Scalar Coordinate type to use, default is [[Rational]]"
                          "# @return ValuatedMatroid<Addition, Scalar> The matroid with a trivial valuation",
                          "trivial_valuation<Addition, Scalar=Rational>(Matroid)");
} }
