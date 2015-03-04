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
#include "polymake/polytope/hypersimplex.h"
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/Array.h"
#include "polymake/PowerSet.h"
#include "polymake/internal/PlainParser.h"
#include "polymake/group/group_domain.h"

namespace polymake { namespace polytope {

perl::Object hypersimplex(int k, int d, perl::OptionSet options)
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

   Array<std::string> labels(n);
   int i(0);
   Matrix<Rational> Vertices(n,d+1);
   Rows< Matrix<Rational> >::iterator v=rows(Vertices).begin();
   Subsets_of_k<sequence> enumerator(range(0,d-1), k);
   for (Subsets_of_k<sequence>::iterator s=entire(enumerator); !s.at_end(); ++s, ++v,++i) {
      (*v)[0]=1;
      v->slice(1).slice(*s).fill(1);
      labels[i] = pm::convToString< Set<int> >()(*s);
   }

   // generate the combinatorial symmetry group on the vertices
   bool group_flag = options["group"];
   if ( group_flag ) {
      perl::Object g("group::GroupOfPolytope");
      g.set_description() << "full combinatorial group on coordinates of " <<  "(" << k << "," << d << ")-hypersimplex" << endl;
      g.set_name("fullCombinatorialGroupOnCoords");
      g.take("DOMAIN") << polymake::group::OnCoords;
      Array< Array< int > > gens(2);
      Array< int > gen = sequence(0,d);
      gen[0]=1;
      gen[1]=0;
      gens[0]=gen;

         
      gen[0]=d-1;
      for ( int j=1; j<=d-1; ++j ) {
         gen[j]=j-1; 
      }
      gens[1]=gen;

      g.take("GENERATORS") << gens;
      p.take("GROUP") << g;

   }


   p.take("VERTEX_LABELS") << labels;
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
                  "# @option Bool group"
                  "# @return Polytope",
                  &hypersimplex, "hypersimplex($,$;{group=>undef})");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
