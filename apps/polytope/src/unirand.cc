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
#include "polymake/Rational.h"
#include "polymake/Map.h"
#include "polymake/PowerSet.h"
#include "polymake/Array.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/linalg.h"
#include "polymake/RandomGenerators.h"
#include <cmath>

namespace polymake { namespace polytope {

BigObject unirand(BigObject p_in, Int n_points_out, OptionSet options)
{
   const bool bounded=p_in.give("BOUNDED");
   if (!bounded)
      throw std::runtime_error("unirand: input polyhedron must be bounded");

   const Matrix<Rational> V = p_in.give("VERTICES");
   const Int d = V.cols(), dim = p_in.call_method("DIM");
   if (dim != d-1) {
      throw std::runtime_error("unirand: polytope must be full-dimensional");
   }

   Rational total_volume(0);
   // partial_volume[i] = sum(volume(simplex[j])), j=[0..i]
   Map<Rational, const Set<Int>> partial_volume;

   const bool boundary=options["boundary"];

   const RandomSeed seed(options["seed"]);
   UniformlyRandom<Rational> rg(seed);
   UniformlyRandom<Rational>::iterator random=rg.begin();

   BigObject p_out("Polytope<Rational>");

   if (boundary) {
      p_out.set_description() << "Uniformly distributed random points from the boundary of " << p_in.name() << "; seed=" << seed.get() << endl;

      typedef Array<Set<Int>> triangulation;
      const triangulation Triangulation=p_in.give("TRIANGULATION.BOUNDARY.FACETS");
      const triangulation f_triangs=p_in.give("TRIANGULATION.BOUNDARY.FACET_TRIANGULATIONS");
      Matrix<Rational> F=p_in.give("FACETS");
      F.col(0).fill(0);

      Rows< Matrix<Rational> >::iterator f=rows(F).begin();
      for (auto fs=entire(f_triangs); !fs.at_end();  ++fs, ++f) {
         const Rational approx_f_norm{ sqrt(double(sqr(*f))) };
         for (auto s=entire(*fs); !s.at_end(); ++s) {
            partial_volume.push_back(total_volume, Triangulation[*s]);
            total_volume += abs(det( V.minor(Triangulation[*s],All) / *f )) / approx_f_norm;
         }
      }
   } else {
      p_out.set_description() << "Uniformly distributed random inner points of " << p_in.name() << "; seed=" << seed.get() << endl;

      typedef Array<Set<Int>> triangulation;
      const triangulation Triangulation=p_in.give("TRIANGULATION.FACETS");

      for (auto s=entire(Triangulation); !s.at_end(); ++s) {
         partial_volume.push_back(total_volume, *s);
         total_volume += abs(det( V.minor(*s,All) ));
      }
   }

   // create n_points-many random points
   Matrix<Rational> points_out(n_points_out, d);
   Vector<Rational> part_1(d-boundary);

   for (auto p_i=entire(rows(points_out)); !p_i.at_end(); ++p_i) {
      // choose the simplex randomly
      const Set<Int>& simplex = partial_volume.find_nearest((*random)*total_volume, operations::le())->second;

      // produce a random partition of 1 for the simplex vertices
      copy_range(random, entire(part_1));
      part_1 /= accumulate(part_1, operations::add());

      *p_i= part_1 * V.minor(simplex, All);
   }

   p_out.take("POINTS") << points_out;
   return p_out;
}

UserFunction4perl("# @category Producing a polytope from polytopes"
                  "# Produce a polytope with //n// random points that are"
                  "# uniformly distributed within the given polytope //P//."
                  "# //P// must be bounded and full-dimensional."
                  "# @param Polytope P"
                  "# @param Int n the number of random points"
                  "# @option Bool boundary forces the points to lie on the boundary of the given polytope"
                  "# @option Int seed controls the outcome of the random number generator;"
                  "#   fixing a seed number guarantees the same outcome."
                  "# @return Polytope"
                  "# @example This produces a polytope as the convex hull of 5 random points in the square with the origin as"
                  "# its center and side length 2:"
                  "# > $p = unirand(cube(2),5);"
                  "# @example This produces a polytope as the convex hull of 5 random points on the boundary of the square with the origin as"
                  "# its center and side length 2:"
                  "# > $p = unirand(cube(2),5,boundary=>1);",
                  &unirand,"unirand(Polytope $ {seed => undef, boundary => 0})");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
