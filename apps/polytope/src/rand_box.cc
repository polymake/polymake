/* Copyright (c) 1997-2022
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
#include "polymake/Matrix.h"
#include "polymake/RandomGenerators.h"

namespace polymake { namespace polytope {

BigObject rand_box(const Int d, const Int n, const Int b, OptionSet options)
{
   if (d<1 || n<1 || b<1)
      throw std::runtime_error("rand_box: 1 <= dim, #POINTS, b");

   const RandomSeed seed(options["seed"]);
   UniformlyRandom<Integer> random(seed);
   const auto start_seed = seed.get();

   Matrix<Rational> V(n, d+1);
   V.col(0).fill(1);
   for (Int i = 0; i < n; ++i)
      for (Int k = 1; k <= d; ++k)
         V(i,k) = random.get() % (b+1);

   BigObject p("Polytope<Rational>",
               "CONE_AMBIENT_DIM", d+1,
               "POINTS", V);
   p.set_description() << "Produced by rand_box for b=" << b << ", seed=" << start_seed << endl;
   return p;
}

UserFunction4perl("# @category Producing a polytope from scratch"
                  "# Computes the convex hull of //n// points sampled uniformly at random from the"
                  "# integer points in the cube [0,//b//]<sup>//d//</sup>."
                  "# @param Int d the dimension of the box"
                  "# @param Int n the number of random points"
                  "# @param Int b the size of the box"
                  "# @option Int seed controls the outcome of the random number generator;"
                  "#   fixing a seed number guarantees the same outcome."
                  "# @return Polytope",
                  &rand_box,"rand_box($$$ { seed => undef })");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
