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
#include "polymake/Vector.h"
#include "polymake/Matrix.h"
#include "polymake/Array.h"
#include "polymake/list"

namespace polymake { namespace tropical {

perl::Object poly2trop(perl::Object p)
{
   const bool bounded=p.give("BOUNDED");

   if (!bounded)
      throw std::runtime_error("input polyhedron not bounded");

   Matrix<Rational> V=p.give("VERTICES | POINTS");
   V.col(0)=zero_vector<Rational>(V.rows());
  
   perl::Object t("TropicalPolytope");
   t.set_description() << "Tropical polytope of " << p.name() <<endl;
 
   t.take("POINTS") << V;
   return t;
}

UserFunction4perl("# @category Producing a tropical polytope"
                  "# Takes an ordinary convex polytope and interprets it in tropical projective space."
                  "# @param polytope::Polytope P"
                  "# @return TropicalPolytope",
                  &poly2trop, "poly2trop(polytope::Polytope)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
