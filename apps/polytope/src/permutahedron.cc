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
#include <algorithm>
#include "polymake/Array.h"

namespace polymake { namespace polytope {

perl::Object permutahedron(int d, perl::OptionSet options)
{
   if (d < 2) {
      throw std::runtime_error("permutahedron: dimension >= 2 required\n");
   }
   perl::Object p("Polytope<Rational>");
   p.set_description() << "permutahedron of dimension " << d << endl;
   // enumerate the (d+1)! vertices of the d-permutahedron
   Matrix<Rational> V(int(Integer::fac(d+1)), d+2);
   Vector<int> perm(d+1, sequence(1).begin());

   Rows<Matrix<Rational>>::iterator r=rows(V).begin();
   do {
      *r = 1 | perm;
      ++r;
   } while (std::next_permutation(perm.begin(),perm.end()));

// generate the combinatorial symmetry group on the coordinates
   const bool group_flag = options["group"];
   if ( group_flag ) {
      Array<Array<int>> gens(2);
      Array<int> gen{sequence(0,d+1)};
      gen[0]=1;
      gen[1]=0;
      gens[0]=gen;

         
      gen[0]=d;
      for (int j=1; j<=d; ++j) {
         gen[j]=j-1; 
      }
      gens[1]=gen;
      
      perl::Object a("group::PermutationAction");
      a.take("GENERATORS") << gens;
      
      perl::Object g("group::Group");
      g.set_description() << "full combinatorial group on coordinates of " << d << "-dim permutahedron" << endl;
      g.set_name("fullCombinatorialGroupOnCoords");

      p.take("GROUP") << g;
      p.take("GROUP.COORDINATE_ACTION") << a;

   }

   p.take("CONE_AMBIENT_DIM") << d+2;
   p.take("CONE_DIM") << d+1;
   p.take("VERTICES") << V;
   p.take("N_VERTICES") << V.rows();
   p.take("BOUNDED") << true;
   return p;
}

perl::Object signed_permutahedron(int d)
{
   if (d < 1) {
      throw std::runtime_error("permutahedron: dimension >= 2 required\n");
   }
   perl::Object p("Polytope<Rational>");
   p.set_description() << "signed permutahedron of dimension " << d << endl;
   const int n(Integer::fac(d) << d);

   // enumerate the d! vertices of the (d-1)-permutahedron
   Matrix<Rational> V(n, d+1);
   Vector<int> perm(d, sequence(1).begin());

   Rows<Matrix<Rational>>::iterator r=rows(V).begin();
   const int m=1<<d;
   do {
      Vector<int> signed_perm(perm);
      for (int i=0; i<m; ++i) {
         *r = 1 | signed_perm;
         ++r;
         // add one "bit"
         for (int k=0; k<d; ++k) {
            if (signed_perm[k] < 0)
               signed_perm[k] *= -1;
            else {
               signed_perm[k] *= -1;
               break;
            }
         }
      }
   } while (std::next_permutation(perm.begin(),perm.end()));

   p.take("CONE_AMBIENT_DIM") << d+1;
   p.take("CONE_DIM") << d+1;
   p.take("VERTICES") << V;
   p.take("N_VERTICES") << V.rows();
   p.take("BOUNDED") << true;
   return p;
}

UserFunction4perl("# @category Producing a polytope from scratch"
                  "# Produce a //d//-dimensional permutahedron."
                  "# The vertices correspond to the elements of the symmetric group of degree //d//+1."
                  "# @param Int d the dimension"
                  "# @option Bool group"
                  "# @return Polytope"
                  "# @example To create the 3-permutahedron and also compute its symmetry group, do this:"
                  "# > $p = permutahedron(3,group=>1);"
                  "# > print $p->GROUP->GENERATORS;"
                  "# | 1 0 2 3"
                  "# | 3 0 1 2",
                  &permutahedron, "permutahedron($,{group=>undef})");

UserFunction4perl("# @category Producing a polytope from scratch"
                  "# Produce a //d//-dimensional signed permutahedron."
                  "# @param Int d the dimension"
                  "# @return Polytope",
                  &signed_permutahedron, "signed_permutahedron($)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
