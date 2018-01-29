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
#include "polymake/IncidenceMatrix.h"

namespace polymake { namespace polytope {


perl::Object reduced(Rational t, Rational x, Rational s, Rational h, Rational r)
{
   if ((s<0.09) || (s>0.2) || (h<0.09) || (h>0.2)
       || (r<0.3) || (r>0.4) || (x<0.6) || (x>0.7)|| (t<0.5) || (t>0.6))
      throw std::runtime_error("reduced: s,h in [0.09,0.2], r in [0.3,0.4], x in [0.6,0.7], t in [0.5,0.6]");
   
   perl::Object p("Polytope<Rational>");
   p.set_description() << "reduced 3-polytope with t=" << t << " x=" << x << " s=" << s << " h=" << h << " r=" << r << endl;

   Matrix<Rational> V(12,4);
   V.col(0) = ones_vector<Rational>(12);

   V(0,1) = r;  V(0,3) = -t;
   V(1,1) = -r; V(1,3) = -t;
   V(2,2) = r;  V(2,3) = t;
   V(3,2) = -r; V(3,3) = t;

   V(4,1) = h;   V(4,2) = x;  V(4,3) = s;
   V(5,1) = -h;  V(5,2) = x;  V(5,3) = s;
   V(6,1) = h;   V(6,2) = -x; V(6,3) = s;
   V(7,1) = -h;  V(7,2) = -x; V(7,3) = s;

   V(8,1) = x;   V(8,2) = h;   V(8,3) = -s;
   V(9,1) = x;   V(9,2) = -h;  V(9,3) = -s;
   V(10,1) = -x; V(10,2) = h;  V(10,3) = -s;
   V(11,1) = -x; V(11,2) = -h; V(11,3) = -s;
   
   IncidenceMatrix<> VIF{
      {0,1,4,5}, {0,1,6,7}, {2,3,8,9}, {2,3,10,11},
      {0,6,9}, {0,4,8}, {1,5,10}, {1,7,11}, {2,4,8}, {2,5,10}, {3,6,9}, {3,7,11},
      {0,8,9}, {1,10,11}, {2,4,5}, {3,6,7}
   };

   p.take("CONE_AMBIENT_DIM") << 4;
   p.take("CONE_DIM") << 4;
   p.take("VERTICES") << V;
   p.take("LINEALITY_SPACE") << Matrix<Rational>(0, 4);
   p.take("VERTICES_IN_FACETS") << VIF;
   p.take("BOUNDED") << true;

   return p;
}

UserFunction4perl("# @category Producing regular polytopes and their generalizations"
                  "# Produce a 3-dimensional reduced polytope (for suitably chosen parameters)."
                  "# That is, a polytope of constant width which does not properly contain a subpolytope of the same width."
                  "# Construction due to Bernardo González Merino, Thomas Jahn, Alexandr Polyanskii and Gerd Wachsmuth, arXiv:1701.08629"
                  "# @param Rational t"
                  "# @param Rational x"
                  "# @param Rational s"
                  "# @param Rational h"
                  "# @param Rational r"
                  "# @return Polytope<Rational>"
                  "# @example These values yield a reduced 3-polytope (approximately).  The width equals 1."
                  "# > $r = reduced(0.55, 0.6176490959800, 0.1351384931026, 0.0984300252409, 0.3547183586709);",
                  &reduced, "reduced($$$$$)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
