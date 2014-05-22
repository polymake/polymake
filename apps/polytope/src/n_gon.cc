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
#include "polymake/Matrix.h"
#include "polymake/Array.h"
#include "polymake/AccurateFloat.h"
#include "polymake/group/group_domain.h"

namespace polymake { namespace polytope {

      perl::Object n_gon(int n, const Rational& r, perl::OptionSet options)
{
   if ((n < 3) || (r <= 0)) {
      throw std::runtime_error("n_gon: n >= 3 and r > 0 required\n");
   }

   perl::Object p("Polytope<Rational>");
   p.set_description() << n << "-gon with radius " << r << endl;
   
   Matrix<Rational> V(n,3);
   V.col(0).fill(1);

   V(0,1)=r;
   if (!(n%2)) {
      V(n/2,1)=-r;
      if (!(n%4)) {
         V(n/4,2)=r;
         V((3*n)/4,2)=-r;
      }
   }

   const int iend= n%2 ? (n+1)/2 : (n+2)/4;
   const AccurateFloat angle = (2 * AccurateFloat::pi()) / n;
   AccurateFloat c, s;
   Rational x, y;
   for (int i=1; i<iend; ++i) {
      sin_cos(s, c, i*angle);
      x=r*c;
      y=r*s;
      V(i,1)=x;
      V(i,2)=y;
      V(n-i,1)=x;
      V(n-i,2)=-y;
      if (!(n%2)) {
         V(n/2-i,1)=-x;
         V(n/2-i,2)=y;
         V(n/2+i,1)=-x;
         V(n/2+i,2)=-y;
      }
   }


   bool group_flag = options["group"];
   if ( group_flag ) {
      perl::Object g("group::GroupOfPolytope");
      g.set_description() << "full combinatorial group on vertices" << endl;
      g.set_name("fullCombinatorialGroupOnRays");
      g.take("DOMAIN") << polymake::group::OnRays;
      Array< Array< int > > gens(2);
      Array< int > gen1(n);
      Array< int > gen2(n);
      for ( int i=0; i<n; ++i ) { // iterating over entries
         gen1[n-i-1]=(i+1)%n;
         gen2[i]=(i+1)%n;
      }

      gens[0]=gen1;
      gens[1]=gen2;
      g.take("GENERATORS") << gens;
      p.take("GROUP") << g;

   }


   p.take("CONE_AMBIENT_DIM") << 3;
   p.take("CONE_DIM") << 3;
   p.take("VERTICES") <<  V;
   p.take("LINEALITY_SPACE") << Matrix<Rational>();
   p.take("N_VERTICES") << n;
   p.take("BOUNDED") << true;
   p.take("CENTERED") << true;
   return p;
}

UserFunction4perl("# @category Producing a polytope from scratch"
                  "# Produce a regular //n//-gon."
                  "# All vertices lie on a circle of radius //r//."
                  "# The radius defaults to 1."
                  "# @param Int n the number of vertices"
                  "# @param Rational r the radius"
                  "# @option Bool group"
                  "# @return Polytope",
                  &n_gon,"n_gon($;$=1, {group=>undef})");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
