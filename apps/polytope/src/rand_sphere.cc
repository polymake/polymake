/* Copyright (c) 1997-2019
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
#include "polymake/Vector.h"
#include "polymake/RandomSpherePoints.h"

namespace polymake { namespace polytope {
template<typename Num>
perl::Object rand_sphere(int d, int n, perl::OptionSet options)
{
   if (d<2 || n<=d) {
      throw std::runtime_error("rand_sphere: 2 <= dim < #vertices\n");
   }
   const RandomSeed seed(options["seed"]);
   const bool use_prec = options.exists("precision");
   const int my_prec = use_prec ? options["precision"] : 0;
   if (use_prec && my_prec < MPFR_PREC_MIN)
      throw std::runtime_error("rand_sphere: MPFR precision too low ( < MPFR_PREC_MIN )");

   perl::Object p("Polytope<Rational>");
   p.set_description() << "Random spherical polytope of dimension " << d << "; seed=" << seed.get()
      << "; precision=" << (use_prec ? std::to_string(my_prec) : "default") << endl;

   RandomSpherePoints<Num> random_source(d, seed);
   if (use_prec)
      random_source.set_precision(my_prec);

   Matrix<Rational> Points(n, d+1);
   Points.col(0).fill(1);
   copy_range(random_source.begin(), entire(rows(Points.minor(All, range(1,d)))));

   p.take("POINTS") << Points;
   p.take("CONE_AMBIENT_DIM") << d+1;
   p.take("BOUNDED") << true;
   return p;
}

UserFunctionTemplate4perl("# @category Producing a polytope from scratch"
                  "# Produce a rational //d//-dimensional polytope with //n// random vertices"
                  "# approximately uniformly distributed on the unit sphere."
                  "# @tparam Num can be AccurateFloat (the default) or Rational"
                  "# With [[AccurateFloat]] the distribution should be closer to uniform,"
                  "# but the vertices will not exactly be on the sphere."
                  "# With [[Rational]] the vertices are guaranteed to be on the unit sphere,"
                  "# at the expense of both uniformity and log-height of points."
                  "# @param Int d the dimension of sphere"
                  "# @param Int n the number of random vertices"
                  "# @option Int seed controls the outcome of the random number generator;"
                  "#   fixing a seed number guarantees the same outcome. "
                  "# @option Int precision Number of bits for MPFR sphere approximation"
                  "# @return Polytope<Rational>",
                  "rand_sphere<Num=AccurateFloat>($$ { seed => undef, precision => undef })");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
