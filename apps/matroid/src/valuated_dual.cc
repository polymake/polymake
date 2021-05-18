/* Copyright (c) 1997-2021
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
#include "polymake/Vector.h"
#include "polymake/Matrix.h"
#include "polymake/Array.h"
#include "polymake/Set.h"
#include "polymake/TropicalNumber.h"



namespace polymake { namespace matroid {
  
//FIXME: Write dualizing method, keeping as much data as possible
// and using matroid's dual(..)?
  
template <typename Addition, typename Scalar>
BigObject dual(BigObject vm)
{
  // Extract values
  const Int n = vm.give("N_ELEMENTS");
  const Array<Set<Int>> bases = vm.give("BASES");
  Vector<TropicalNumber<Addition,Scalar> > valuation = vm.give("VALUATION_ON_BASES");

  // Convert bases
  Array<Set<Int>> dual_bases(bases.size());
  for (Int b = 0; b < bases.size(); ++b) {
    dual_bases[b] = sequence(0,n) - bases[b];
  }

  return BigObject(vm.type(),
                   "N_ELEMENTS", n,
                   "BASES", dual_bases,
                   "VALUATION_ON_BASES", valuation);
}

UserFunctionTemplate4perl("# @category Producing a matroid from matroids"
                          "# Computes the dual of a valuated matroid."
                          "# @param ValuatedMatroid<Addition,Scalar> M A valuated matroid"
                          "# @return ValuatedMatroid<Addition,Scalar> The dual valuated matroid.",
                          "dual<Addition,Scalar>(ValuatedMatroid<Addition,Scalar>)");

} }
