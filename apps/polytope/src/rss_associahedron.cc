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
#include "polymake/vector"
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/ListMatrix.h"
#include <sstream>

namespace polymake { namespace polytope {

perl::Object rss_associahedron(const int n)
{
   if (n<2) {
      throw std::runtime_error("rss_associahedron: n>=2\n");
   }

   const int m=(n*(n-1))/2-1; // number of facets
   Matrix<Rational> I(m,n+1); // initialized as zero matrix
   std::vector<std::string> facet_labels(m);
   int k=0;
   for (int i=1; i<=n; ++i)
      for (int j=i+1; j<=n; ++j) {
         if (i!=1 || j!=n) {
            I(k,0) = -(i-j)*(i-j);
            I(k,i) = -1; I(k,j) = 1;
            facet_labels[k] = std::to_string(i) + "," + std::to_string(j);
            ++k;
         }
      }

   Matrix<Rational> normalizing_equations(2,n+1);
   normalizing_equations(0,1)=1;
   normalizing_equations(1,0)=-(n-1)*(n-1); normalizing_equations(1,1)=-1; normalizing_equations(1,n)=1;

   perl::Object p("Polytope<Rational>");
   p.take("FACETS") << I;
   p.take("AFFINE_HULL") << normalizing_equations;
   p.take("FACET_LABELS") << facet_labels;

   return p;
}

UserFunction4perl("# @category Producing a polytope from scratch"
                  "# Produce a polytope of constrained expansions in dimension //l// according to"
                  "#\t Rote, Santos, and Streinu: Expansive motions and the polytope of pointed pseudo-triangulations."
                  "#\t Discrete and computational geometry, 699--736, Algorithms Combin., 25, Springer, Berlin, 2003."
                  "# @param Int l ambient dimension"
                  "# @return Polytope",
                  &rss_associahedron,"rss_associahedron");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
