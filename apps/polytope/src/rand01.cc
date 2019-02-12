/* Copyright (c) 1997-2018
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
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/RandomGenerators.h"
#include "polymake/hash_set"

namespace polymake { namespace polytope {

perl::Object rand01(int d, int n, perl::OptionSet options)
{
   // The behaviour of the shift operator is undefined if the second argument is larger (or equal) than the bitsize of the first,
   // in that case the test is not necessary.
   if (d<2 || n<=d || (d <= std::numeric_limits<int>::digits && (n-1>>d)>=1))
      throw std::runtime_error("rand01 : 2 <= dim < #vertices <= 2^dim required");

   const RandomSeed seed(options["seed"]);
   UniformlyRandom<Bitset> random(d,seed);

   perl::Object p("Polytope<Rational>");
   p.set_description() << "Random 0-1 polytope; seed=" << seed.get() << endl;

   p.take("CONE_AMBIENT_DIM") << d+1;
   p.take("N_VERTICES") << n;

   hash_set<Bitset> bitvectors(n);

   while (bitvectors.size() < (size_t)n)
      bitvectors.insert(random.get());

   Matrix<Rational> V(n,d+1);
   hash_set<Bitset>::iterator bv=bitvectors.begin();
   for (auto v=entire(rows(V)); !v.at_end(); ++v, ++bv)
      *v = 1 | same_element_sparse_vector<int>(*bv,d);
      
   p.take("VERTICES") << V;
   Matrix<Rational> empty_lin_space(0,V.cols());	
   p.take("LINEALITY_SPACE") << empty_lin_space;
   p.take("POINTED") << 1;
   p.take("LINEALITY_DIM") << 0;
   return p;
}

UserFunction4perl("# @category Producing a polytope from scratch"
                  "# Produce a //d//-dimensional 0/1-polytope with //n// random vertices."
                  "# Uniform distribution."
                  "# @param Int d the dimension"
                  "# @param Int n the number of random vertices"
                  "# @option Int seed controls the outcome of the random number generator;"
                  "#   fixing a seed number guarantees the same outcome. "
                  "# @return Polytope",
                  &rand01, "rand01($$ { seed => undef })");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
