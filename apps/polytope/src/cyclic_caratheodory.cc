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
#include "polymake/AccurateFloat.h"

namespace polymake { namespace polytope {

perl::Object cyclic_caratheodory(const int d, const int n)
{
   if ((d < 2) || (d >= n)) {
      throw std::runtime_error("cyclic_caratheodory: (d >= 2) && (n > d)\n");
   }
   if (d % 2) {
      throw std::runtime_error("cyclic_caratheodory: even dimension required.\n");
   }
   
   perl::Object p("Polytope<Rational>");
   p.set_description() << "Cyclic " << d << "-polytope on " << n << " vertices on the trigonometric moment curve" << endl;

   Matrix<Rational> Vertices(n,d+1);
   auto v=concat_rows(Vertices).begin();

   AccurateFloat x(0), s, c;
   for (int r=0; r<n; ++r, ++x) {
      *v++ = 1;
      for (int i = 1; i <= d/2; ++i) {
         sin_cos(s, c, x*i);
         *v++ = c;
         *v++ = s;
      }
   }

   p.take("CONE_AMBIENT_DIM") << d+1;
   p.take("CONE_DIM") << d+1;
   p.take("N_VERTICES") << n;
   p.take("VERTICES") << Vertices;
   p.take("BOUNDED") << true;
   return p;
}

UserFunction4perl("# @category Producing a polytope from scratch"
                  "# Produce a //d//-dimensional cyclic polytope with //n// points."
                  "# Prototypical example of a neighborly polytope.  Combinatorics completely known"
                  "# due to Gale's evenness criterion.  Coordinates are chosen on the trigonometric"
                  "# moment curve. For cyclic polytopes from other curves, see [[polytope::cyclic]]."
                  "# @param Int d the dimension. Required to be even."
                  "# @param Int n the number of points"
                  "# @return Polytope",
                  &cyclic_caratheodory,"cyclic_caratheodory($$)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
