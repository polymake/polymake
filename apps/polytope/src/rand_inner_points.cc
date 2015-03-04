/* Copyright (c) 1997-2015
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

#include <limits>
#include "polymake/client.h"
#include "polymake/RandomGenerators.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/Set.h"

namespace polymake { namespace polytope {

perl::Object rand_inner_points(perl::Object p_in, int n, perl::OptionSet options)
{
   const bool bounded=p_in.give("BOUNDED");
   if (!bounded)
      throw std::runtime_error("rand_inner_points: unbounded input polyhedron");

   const RandomSeed seed(options["seed"]);
   const long M=std::numeric_limits<int>::max();
   UniformlyRandomRanged<long> rg(M,seed);

   perl::Object p_out("Polytope<Rational>");
   p_out.set_description() << "Random inner points of " << p_in.name() << "; seed=" << seed.get() << endl;

   const Matrix<Rational> V=p_in.give("VERTICES");
   const int d=V.cols(), n_vertices=V.rows();

   Vector<Rational> mu(n_vertices);
   Matrix<Rational> Points_out(n,d);
   for (Entire< Rows< Matrix<Rational> > >::iterator p_i=entire(rows(Points_out)); !p_i.at_end(); ++p_i) {
      // get random partition of 1
      rg.begin();
      Set<int> partition;
      for (int i=1; i<n_vertices; ++i)
         partition+=abs(rg.get());  // FIXME: check if already contained
      Rational previous(0);
      int idx=0;
      for ( Entire< Set<int> >::iterator pit=entire(partition); !pit.at_end(); ++pit, ++idx ) {
         const Rational current(*pit);
         mu[idx]=(current-previous)/M;
         previous=current;
      }
      mu[idx]=1-(previous/M);
      // crude fix for FIXME above; make sure that the vector always has the proper length
      for ( ++idx; idx<n_vertices; ++idx ) mu[idx]=0;

      *p_i = mu*V;
   }
   
   p_out.take("POINTS") << Points_out;
   return p_out;
}

UserFunction4perl("# @category Producing a polytope from polytopes"
                  "# Produce a polytope with //n// random points from the input polytope //P//."
                  "# Each generated point is a convex linear combination of the input vertices"
                  "# with uniformly distributed random coefficients. Thus, the output points can't in general"
                  "# be expected to be distributed uniformly within the input polytope; cf. [[unirand]] for this."
                  "# The polytope must be [[BOUNDED]]."
                  "# @param Polytope P the input polytope"
                  "# @param Int n the number of random points"
                  "# @option Int seed controls the outcome of the random number generator;"
                  "#   fixing a seed number guarantees the same outcome."
                  "# @return Polytope"
                  "# @author Carsten Jackisch",
                  &rand_inner_points, "rand_inner_points(Polytope $ { seed => undef })");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
