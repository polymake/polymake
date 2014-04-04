/* Copyright (c) 1997-2014
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
#include "polymake/polytope/hypersimplex.h"
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/PowerSet.h"

namespace polymake { namespace polytope {

perl::Object hypersimplex(int k, int d)
{
   if (d < 1)
      throw std::runtime_error("hypersimplex: dimension >= 1 required");
   if (k < 0 || k > d)
      throw std::runtime_error("hypersimplex: 0 <= k <= d required");

   perl::Object p("Polytope<Rational>");
   p.set_description() << "(" << k << "," << d << ")-hypersimplex" << endl;

   p.take("CONE_AMBIENT_DIM") << d+1;
   p.take("CONE_DIM") << d;

   // we already know the number of vertices
   const int n=Integer::binom(d,k).to_int();
   p.take("N_VERTICES") << n;

   Matrix<Rational> Vertices(n,d+1);
   Rows< Matrix<Rational> >::iterator v=rows(Vertices).begin();
   Subsets_of_k<sequence> enumerator(range(1,d), k);
   for (Subsets_of_k<sequence>::iterator s=entire(enumerator); !s.at_end(); ++s, ++v) {
      (*v)[0]=1;
      v->slice(*s).fill(1);
   }
   p.take("VERTICES") << Vertices;
   p.take("LINEALITY_SPACE") << Matrix<Rational>();

   return p;
}

UserFunction4perl("# @category Producing a polytope from scratch"
                  "# Produce the hypersimplex &Delta;(//k//,//d//), that is the the convex hull of all 0/1-vector in R<sup>//d//</sup>"
                  "# with exactly //k// 1s."
                  "# Note that the output is never full-dimensional."
                  "# @param Int k number of 1s"
                  "# @param Int d ambient dimension"
                  "# @return Polytope",
                  &hypersimplex, "hypersimplex");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
