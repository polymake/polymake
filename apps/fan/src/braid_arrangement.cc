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
#include "polymake/PowerSet.h"
#include "polymake/SparseMatrix.h"

namespace polymake { namespace fan {

BigObject braid_arrangement(Int d)
{
   if (d < 2)
      throw std::runtime_error("braid_arrangement: dimension >= 2 required");

   BigObject HA("HyperplaneArrangement<Rational>");
   HA.set_description() << "Braid arrangement of dimension " << d << endl;

   const Int n(Integer::binom(d,2));
   HA.take("N_HYPERPLANES") << n;
   HA.take("HYPERPLANE_AMBIENT_DIM") << d;

   // orientation such that x_0 < x_1 < ... < x_d is the positive cell
   SparseMatrix<Rational> Hyperplanes(n,d);
   Int i=0;
   for (auto pair=entire(all_subsets_of_k(sequence(0,d),2)); !pair.at_end(); ++pair, ++i) {
      Hyperplanes(i,pair->front()) = -1;
      Hyperplanes(i,pair->back()) = 1;
   }
   HA.take("HYPERPLANES") << Hyperplanes;

   Matrix<Rational> Lineality(1,d);
   Lineality[0] = ones_vector<Rational>(d);
   HA.take("LINEALITY_SPACE") << Lineality;

   return HA;
}


UserFunction4perl("# @category Producing a hyperplane arrangement"
                  "# Produce the braid arrangement in dimension $d$"
                  "# @param Int d ambient dimension"
                  "# @return HyperplaneArrangement"
                  "# @example"
                  "# > $B = braid_arrangement(3);",
                  &braid_arrangement, "braid_arrangement($)");


} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
