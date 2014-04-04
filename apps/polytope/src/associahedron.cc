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
#include "polymake/Rational.h"
#include "polymake/Matrix.h"

namespace polymake { namespace polytope {

perl::Object associahedron(int d)
{
   perl::Object p("Polytope<Rational>");
   p.set_description() << "Associahedron of dimension " << d << endl; 
   const int n = d+2; // ambient dimension

   // two equations
   Matrix<Rational> EQ(2,n+1);
   const Rational
      n2=n*n, n3=n2*n, n4=n3*n, n5=n4*n; // powers of n
   EQ(0,0)=-(n4-n2)/4;
   for (int i=1; i<=n; ++i) EQ(0,i)=i;
   EQ(1,0)=-(6*n5-5*n3-n)/30;
   for (int i=1; i<=n; ++i) EQ(1,i)=i*i;
   p.take("AFFINE_HULL") << EQ;
   
   // facets
   Matrix<Rational> F(n*(n-1)/2-1, n+1);
   Rows< Matrix<Rational> >::iterator f=rows(F).begin();
   for (int i=0; i<n-1; ++i)
      for (int j=i+2; j<=n-(i==0); ++j, ++f) {
         (*f)[0]=-Integer::binom(j-i+1,3)*Rational(3*(j-i)*(j-i)-2, 10);
         for (int k=i+1; k<j; ++k) (*f)[k]=(k-i)*(j-k);
      }
   p.take("FACETS") << F;
   p.take("CONE_AMBIENT_DIM") << n+1;
   p.take("CONE_DIM") << d+1;
   return p;
}

UserFunction4perl("# @category Producing a polytope from scratch"
                  "# Produce a //d//-dimensional associahedron (or Stasheff polytope)."
                  "# We use the facet description given in Ziegler's book on polytopes, section 9.2."
                  "# @param Int d the dimension"
                  "# @return Polytope",
                  &associahedron, "associahedron($)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
