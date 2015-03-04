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

#include "polymake/client.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/RandomSpherePoints.h"
#include "polymake/Set.h"
#include "polymake/Array.h"
#include "polymake/list"

namespace polymake { namespace topaz {

perl::Object rand_knot(const int n_edges, perl::OptionSet options)
{
   if (n_edges < 3)
      throw std::runtime_error("rand_knot: less than 3 edges.\n");
  
   const int n_comp = options["n_comp"];

   perl::Object p("GeometricSimplicialComplex<Rational>");
   p.set_description() << "A random knot/link with " << n_comp
                       << " components with " << n_edges << " edges each.\n";

   std::list< Set<int> > C;
   for (int i=0; i<n_comp; ++i) {
      for (int j=i*n_edges; j<(i+1)*n_edges-1; ++j)
         C.push_back(sequence(j,2));
      Set<int> e;
      e+=i*n_edges;  e+=(i+1)*n_edges-1;
      C.push_back(e);
   }

   const RandomSeed seed(options["seed"]);
  
   Matrix<Rational> Points(n_edges*n_comp, 3);
   if (options["on_sphere"] || options["brownian"]) {
      RandomSpherePoints<> random_source(3, seed);
      copy(random_source.begin(), entire(rows(Points)));
    
      if (options["brownian"])
         for (int i=1; i<Points.rows(); ++i)
           Points[i] += Points[i-1];

   } else {
      UniformlyRandom<AccurateFloat> rg(seed);
      copy(rg.begin(), entire(concat_rows(Points)));
   }

   p.take("FACETS") << C;
   p.take("COORDINATES") << Points;
   return p;
}

UserFunction4perl("# @category Producing from scratch\n"
                  "# Produce a random knot (or link) as a polygonal closed curve in 3-space.\n"
                  "# The knot (or each connected component of the link) has //n_edges// edges.\n"
                  "# "
                  "# The vertices are uniformly distributed in [-1,1]<sup>3</sup>, unless the //on_sphere// option is set.\n"
                  "# In the latter case the vertices are uniformly distributed on the 3-sphere. Alternatively\n" 
                  "# the //brownian// option produces a knot by connecting the ends of a simulated brownian motion.\n"
                  "# @param Int n_edges"
                  "# @option Int n_comp number of components, default is 1."
                  "# @option Bool on_sphere"
                  "# @option Bool brownian"
                  "# @option Int seed"
                  "# @return SimplicialComplex",
                  &rand_knot, "rand_knot($ { n_comp => 1,on_sphere => undef, brownian => undef, seed => undef })");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
