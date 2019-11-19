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
#include "polymake/Rational.h"
#include "polymake/Vector.h"
#include "polymake/Matrix.h"
#include "polymake/MultiDimCounter.h"

namespace polymake { namespace polytope {

perl::Object pile(const Vector<int>& sizes)
{
   perl::Object p("Polytope<Rational>");
   p.set_description() << "Lifted pile of cubes" << endl;

   const int d=sizes.size();
   int n=1;
   for (auto s=entire(sizes); !s.at_end(); ++s)
      n *= *s+1;

   Matrix<Rational> V(n,d+2);
   const int factor(d*d);
   Vector<Rational> limits(sizes);
   limits/=2;
   Rows< Matrix<Rational> >::iterator V_i=rows(V).begin();

   for (MultiDimCounter<false,Rational> x(-limits, limits+ones_vector<Rational>(d)); !x.at_end(); ++x, ++V_i)
      *V_i = 1 | *x | (sqr(*x) / factor);

   p.take("VERTICES") << V;
   p.take("N_VERTICES") << n;
   return p;
}

UserFunction4perl("# @category Producing a polytope from scratch"
                  "# Produce a (//d//+1)-dimensional polytope from a pile of cubes."
                  "# Start with a //d//-dimensional pile of cubes.  Take a generic convex function"
                  "# to lift this polytopal complex to the boundary of a (//d//+1)-polytope."
                  "# @param Vector<Int> sizes a vector (s<sub>1</sub>,...,s<sub>d</sub>,"
                  "#   where s<sub>i</sub> specifies the number of boxes in the i-th dimension."
                  "# @return Polytope",
                  &pile, "pile");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
