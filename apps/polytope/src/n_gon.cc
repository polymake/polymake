/* Copyright (c) 1997-2020
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
#include "polymake/Matrix.h"
#include "polymake/Array.h"
#include "polymake/AccurateFloat.h"

namespace polymake { namespace polytope {

BigObject n_gon(Int n, const Rational& r, const Rational& alpha_0, OptionSet options)
{
   if ((n < 3) || (r <= 0)) {
      throw std::runtime_error("n_gon: n >= 3 and r > 0 required\n");
   }

   BigObject p("Polytope<Rational>");
   p.set_description() << n << "-gon with radius " << r << " and initial angle " << alpha_0
                       << (alpha_0.is_zero() ? "" : " pi") << endl;
   
   Matrix<Rational> V(n,3);
   V.col(0).fill(1);

   AccurateFloat c, s;
   sin_cos(s, c, alpha_0 * AccurateFloat::pi());
   V(0,1)=r*c;
   V(0,2)=r*s;
   if (!(n%2)) {
      V(n/2,1)=-V(0,1);
      V(n/2,2)=-V(0,2);
      if (!(n%4)) {
         V(n/4,1)=V(0,2);
         V(n/4,2)=V(0,1);
         V((3*n)/4,1)=-V(0,2);
         V((3*n)/4,2)=-V(0,1);
      }
   }

   const Int iend = n%2 ? (n+1)/2 : (n+2)/4;
   const AccurateFloat angle = (2 * AccurateFloat::pi()) / n;

   for (Int i = 1; i < iend; ++i) {
      sin_cos(s, c, alpha_0*AccurateFloat::pi() + i*angle);
      Rational x(r*c);
      Rational y(r*s);
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

   const bool group_flag = options["group"];
   if (group_flag) {
      Array<Array<Int>> gens(2);
      Array<Int> gen1(n);
      Array<Int> gen2(n);
      for (Int i = 0; i < n; ++i) { // iterating over entries
         gen1[n-i-1]=(i+1)%n;
         gen2[i]=(i+1)%n;
      }

      gens[0]=gen1;
      gens[1]=gen2;

      BigObject a("group::PermutationAction", "GENERATORS", gens);
      BigObject g("group::Group", "fullCombinatorialGroupOnRays");
      g.set_description() << "full combinatorial group on vertices" << endl;
      p.take("GROUP") << g;
      p.take("GROUP.VERTICES_ACTION") << a;
   }

   p.take("CONE_AMBIENT_DIM") << 3;
   p.take("CONE_DIM") << 3;
   p.take("VERTICES") <<  V;
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
                  "# @param Rational r the radius (defaults to 1)"
                  "# @param Rational alpha_0 the initial angle divided by pi (defaults to 0)"
                  "# @option Bool group"
                  "# @return Polytope"
                  "# @example To store the regular pentagon in the variable $p and calculate its symmetry group, do this:"
                  "# > $p = n_gon(5,group=>1);"
                  "# > print $p->GROUP->RAYS_ACTION->GENERATORS;"
                  "# | 0 4 3 2 1"
                  "# | 1 2 3 4 0",
                  &n_gon,"n_gon($;$=1, $=0, {group=>undef})");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
