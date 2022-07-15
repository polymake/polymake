/* Copyright (c) 1997-2022
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
#include "polymake/Array.h"
#include "polymake/Matrix.h"

namespace polymake { namespace polytope {

BigObject associahedron(Int d, OptionSet options)
{
   BigObject p("Polytope<Rational>");
   p.set_description() << "Associahedron of dimension " << d << endl; 
   const Int n = d+2; // ambient dimension

   // two equations
   Matrix<Rational> EQ(2,n+1);
   const Rational
      n2=n*n, n3=n2*n, n4=n3*n, n5=n4*n; // powers of n
   EQ(0,0) = -(n4-n2)/4;
   for (Int i = 1; i <= n; ++i)
      EQ(0,i) = i;
   EQ(1,0) = -(6*n5-5*n3-n)/30;
   for (Int i = 1; i <= n; ++i)
      EQ(1,i) = i*i;
   p.take("AFFINE_HULL") << EQ;

   // facets
   Matrix<Rational> F(n*(n-1)/2-1, n+1);
   Rows< Matrix<Rational> >::iterator f=rows(F).begin();
   for (Int i = 0; i < n-1; ++i)
      for (Int j = i+2; j <= n-(i==0); ++j, ++f) {
         (*f)[0]=-Integer::binom(j-i+1,3)*Rational(3*(j-i)*(j-i)-2, 10);
         for (Int k = i+1; k < j; ++k) (*f)[k]=(k-i)*(j-k);
      }
   p.take("FACETS") << F;
   p.take("CONE_AMBIENT_DIM") << n+1;
   p.take("CONE_DIM") << d+1;

   const bool group_flag = options["group"];
   if (group_flag) {
       //given (i,j), this returns the row index of the corresponding facet in the facet matrix
       auto ind = [d](Int i, Int j) {
          i %= d+3;
          j %= d+3;
          Int k = 0;
          if (j < i) { //swap i and j
             k=i; i=j; j=k; k=0;
          }
          if (i>0)
             k = (i*(2*d-i+3))/2-1;
          k += j-i-2;
          return k;
      };

      //the symmetry group is induced by the digedral group on an n+3-gon
      Array<Array<Int>> gens(2);
      Array<Int> gen0(n*(n-1)/2-1); //generator for rotation: map facet (i,j) to (i+1,j+1) mod n
      Array<Int> gen1(n*(n-1)/2-1); //generator for reflection: map facet (i,j) to (n-i,n-j)

      for (Int i = 0; i < n-1; ++i)
         for (Int j = i+2; j <= n-(i==0); ++j) {
            gen0[ind(i,j)] = ind(i+1,j+1);
            gen1[ind(i,j)] = ind(n-i,n-j);
         }

      gens[0]=gen0;
      gens[1]=gen1;
      BigObject a("group::PermutationAction", "GENERATORS", gens);
      BigObject g("group::Group", "fullCombinatorialGroupOnFacets", "FACETS_ACTION", a);
      g.set_description() << "full combinatorial group on facets" << endl;
      p.take("GROUP") << g;
   }

   return p;
}

UserFunction4perl("# @category Producing a polytope from scratch"
                  "# Produce a //d//-dimensional associahedron (or Stasheff polytope)."
                  "# We use the facet description given in Ziegler's book on polytopes, section 9.2."
                  "# @param Int d the dimension"
                  "# @option Bool group Compute the combinatorial symmetry group of the polytope."
                  "#  It has two generators, as it is induced by the symmetry group of an d+3-gon,"
                  "#  the dihedral group of degree d+3. See arXiv 1109.5544v2 for details."
                  "# @return Polytope",
                  &associahedron, "associahedron($;{group=>undef})");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
