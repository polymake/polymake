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
#include "polymake/Array.h"
#include "polymake/Set.h"
#include "polymake/Matrix.h"
#include "polymake/Integer.h"
#include "polymake/graph/HasseDiagram.h"

namespace polymake { namespace polytope {


void matroid_polytope(perl::Object m,  perl::OptionSet options )
{
  const Array< Set<int> > bases=m.give("BASES");
  const int n_bases=bases.size();
  const int n_elements=m.give("N_ELEMENTS");
  
  perl::Object p("polytope::Polytope<Rational>");
  Matrix<Rational> V(n_bases,n_elements+1);
  
  //test for each subset of size r
  for (int b=0; b<n_bases; ++b) {
    V(b,0)=1;
    for (Entire< Set<int> >::const_iterator i=entire(bases[b]); !i.at_end(); ++i)
      V(b,(*i)+1)=1;
  }

  bool ineq_flag = options["inequalities"];
  if(ineq_flag && m.give("CONNECTED") && n_elements>1){
      const graph::HasseDiagram lattice=m.give("LATTICE_OF_FLATS");
      const int size( lattice.nodes()-2 ); //do not use the bottom and top node
      const int rank=m.give("RANK");
      Matrix<Rational> I(size+2*n_elements,n_elements+1);
      Matrix<Rational> E(1,n_elements+1);
      int f(0);
      for(int j=1 ; j<rank; ++j){
         for (Entire<graph::HasseDiagram::nodes_of_dim_set>::iterator fi=entire(lattice.nodes_of_dim(j)); !fi.at_end(); ++fi,++f) {
            I(f,0)=j;
            for (Entire< Set<int> >::const_iterator i=entire(lattice.face(*fi)); !i.at_end(); ++i)
               I(f,(*i)+1)=-1;
         }
      }
      //hypersimplex
      //0 <= x_i <= 1 + sum x_i = rank :
      E(0,0)=-rank;
      for(int i=0;i<n_elements;++i){
         I(size+2*i,i+1)=1;
         I(size+2*i+1,0)=1;
         I(size+2*i+1,i+1)=-1;
         E(0,i+1)=1;
      }
   p.take("INEQUALITIES") << I;
   p.take("EQUATIONS") << E;
  }

  p.take("VERTICES") << V;
  p.take("CONE_AMBIENT_DIM") << n_elements+1;
  
  m.take("POLYTOPE") << p;
}

InsertEmbeddedRule("REQUIRE_APPLICATION matroid\n\n");

UserFunction4perl("# @category Producing a polytope from other objects"
                  "# Produce the matroid polytope from a matroid //m//."
                  "# Each vertex corresponds to a basis of the matroid,"
                  "# the non-bases coordinates get value 0, the bases coordinates get value 1."
                  "# @param matroid::Matroid m"
                  "# @option Bool inequalities also generate [[INEQUALITIES]], if [[CONNECTED]]"
                  "# @return Polytope<Rational>",
                  &matroid_polytope, "matroid_polytope(matroid::Matroid, { inequalities => undef } )");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
