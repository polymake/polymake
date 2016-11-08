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
#include "polymake/Rational.h"
#include "polymake/Matrix.h"

namespace polymake { namespace polytope {

namespace {

   Matrix<Rational> cyclic_vertices(const int d, const int n, const int x_start)
   {
      Matrix<Rational> vertices(n,d+1);
      auto v=concat_rows(vertices).begin();

      // and now we compute n points (x_i^1,..,x_i^d) on the momentum curve in R^d
      for (int i=0, x=x_start;  i<n;  ++i, ++x) {
         *v++ = 1;
         Integer power_of_x(1);
         for (int j = 1; j <= d; ++j) {
            power_of_x *= x;
            *v++ = power_of_x;
         }
      }

      return vertices;
   }

   /* For the following see:

      R. Seidel: Exact upper bounds for the number of faces in d-dimensional Voronoi diagrams, in
      Applied Geometry and Discrete Mathematics: The Victor Klee Festschrift, vol. 4 of DIMACS
      Ser. Discrete Math. Theoret. Comput. Sci., Amer. Math. Soc., Providence, RI, 1991, p. 517-529

   */
   Matrix<Rational> spherical_cyclic_vertices(const int d, const int n, const int x_start)
   {
      Matrix<Rational> vertices(n,d+1);
      auto v=concat_rows(vertices).begin();

      // and now we compute n points on the spherical momentum curve in R^d
      for (int i=0, x=x_start;  i<n;  ++i, ++x) {

         // compute 1 + x^2 + x^4 + ... x^{2(d-1)}
         Integer even_power_of_x(1);
         Integer sum_of_even_powers_of_x(1);
         for (int j=1; j<d; ++j) {
            even_power_of_x *= x*x;
            sum_of_even_powers_of_x += even_power_of_x;
         }
 
         *v++ = 1;
         Integer power_of_x(1);
         for (int j = 1; j <= d; ++j) {
            *v++ = Rational(power_of_x,sum_of_even_powers_of_x);
            power_of_x *= x;
         }
      }

      return vertices;
   }

}

perl::Object cyclic(const int d, const int n, perl::OptionSet options)
{
   if ((d < 2) || (d >= n)) {
      throw std::runtime_error("cyclic: d >= 2 and n > d required\n");
   }

   int x_start(options["start"]);
   const bool spherical = options["spherical"];

   perl::Object p("Polytope<Rational>");

   Matrix<Rational> vertices;
   if (spherical) {
      p.set_description() << "Spherical cyclic " << d << "-polytope on " << n << " vertices" << endl;
      if (x_start<=0) x_start=1; // must be positive
      vertices=spherical_cyclic_vertices(d,n,x_start);
   } else {
      p.set_description() << "Cyclic " << d << "-polytope on " << n << " vertices" << endl;
      vertices=cyclic_vertices(d,n,x_start);
   }

   p.take("CONE_AMBIENT_DIM") << d+1;
   p.take("CONE_DIM") << d+1;
   p.take("N_VERTICES") << n;
   p.take("VERTICES") << vertices;
   p.take("BOUNDED") << true;
   return p;
}
   
UserFunction4perl("# @category Producing a polytope from scratch"
                  "# Produce a //d//-dimensional cyclic polytope with //n// points."
                  "# Prototypical example of a neighborly polytope.  Combinatorics completely known"
                  "# due to Gale's evenness criterion.  Coordinates are chosen on the (spherical) moment curve"
                  "# at integer steps from //start//, or 0 if unspecified."
                  "# If //spherical// is true the vertices lie on the sphere with center (1/2,0,...,0) and radius 1/2."
                  "# In this case (the necessarily positive) parameter //start// defaults to 1."
                  "# @param Int d the dimension"
                  "# @param Int n the number of points"
                  "# @option Int start defaults to 0 (or to 1 if spherical)"
                  "# @option Bool spherical defaults to false"
                  "# @return Polytope"
                  "# @example To create the 2-dimensional cyclic polytope with 6 points on the sphere, starting at 3:"
                  "# > $p = cyclic(2,6,start=>3,spherical=>1);"
                  "# > print $p->VERTICES;"
                  "# | 1 1/10 3/10"
                  "# | 1 1/26 5/26"
                  "# | 1 1/37 6/37"
                  "# | 1 1/50 7/50"
                  "# | 1 1/65 8/65",
                  &cyclic, "cyclic($$ { start => 0, spherical => 0})");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
