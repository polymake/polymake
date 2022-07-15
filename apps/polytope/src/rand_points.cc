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
#include "polymake/Vector.h"
#include "polymake/RandomPoints.h"

namespace polymake { namespace polytope {
template <typename Generator>
BigObject rand_points(Int d, Int n, OptionSet options, std::string description)
{
   if (d < 2 || n <= d) {
      throw std::runtime_error("rand_points: 2 <= dim < #vertices\n");
   }
   const RandomSeed seed(options["seed"]);
   const auto start_seed = seed.get();

   const bool use_prec = options.exists("precision");
   const int my_prec = use_prec ? options["precision"] : 0;
   if (use_prec && my_prec < MPFR_PREC_MIN)
      throw std::runtime_error("rand_points: MPFR precision too low ( < MPFR_PREC_MIN )");

   Generator random_source(d, seed);
   if (use_prec)
      random_source.set_precision(my_prec);

   Matrix<Rational> Points(n, d+1);
   Points.col(0).fill(1);
   copy_range(random_source.begin(), entire(rows(Points.minor(All, range(1,d)))));

   BigObject p("Polytope<Rational>",
               "POINTS", Points,
               "CONE_AMBIENT_DIM", d+1,
               "BOUNDED", true);
   p.set_description() << description << " " << d << "; seed=" << start_seed
                       << "; precision=" << (use_prec ? std::to_string(my_prec) : "default") << endl;
   return p;
}

template <typename Num>
BigObject rand_sphere(Int d, Int n, OptionSet options)
{
	BigObject result = rand_points<RandomSpherePoints<Num>>(d, n, options, "Random spherical polytope of dimension");
	return result;
}

template <typename Num>
BigObject rand_normal(Int d, Int n, OptionSet options)
{
	return rand_points<RandomNormalPoints<Num>>(d, n, options, "Random normal polytope of dimension");
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

UserFunctionTemplate4perl("# @category Producing a polytope from scratch"
                  "# Produce a rational //d//-dimensional polytope from //n// random points"
                  "# approximately normally distributed in the unit ball."
                  "# @param Int d the dimension of ball"
                  "# @param Int n the number of random points"
                  "# @option Int seed controls the outcome of the random number generator;"
                  "#   fixing a seed number guarantees the same outcome. "
                  "# @option Int precision Number of bits for MPFR sphere approximation"
                  "# @return Polytope<Rational>",
                  "rand_normal<Num=AccurateFloat>($$ { seed => undef, precision => undef })");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
