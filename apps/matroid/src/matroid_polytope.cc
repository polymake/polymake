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
#include "polymake/Array.h"
#include "polymake/Set.h"
#include "polymake/Matrix.h"
#include "polymake/Integer.h"

namespace polymake { namespace matroid {

Set<int> matroid_indices_of_hypersimplex_vertices(perl::Object m)
{
   const Array< Set<int> > bases=m.give("BASES");
   const int n=m.give("N_ELEMENTS");
   const int d=m.give("RANK");
   Set<int> set;
   int temp_d;
   int temp;
   for (Entire< Array< Set<int> > >::const_iterator b=entire(bases); !b.at_end(); ++b){
      int sum=0;
      temp_d=d;
      temp=0;
      for (Entire< Set<int> >::const_iterator i=entire(*b); !i.at_end(); ++i){
         if(temp_d==d && *i!=0)
            sum+=Integer::binom(n-1,d-1).to_int();
         --temp_d;
         for(int k=1;k<=*i-temp-1;++k)
            sum+=Integer::binom(n-temp-1-k,temp_d).to_int();
         temp=*i;
      }
      set+=sum;
   }
   return set;
}



void matroid_polytope(perl::Object m)
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

  Array< Set<int> > mat_hy;
   if(m.lookup("MATROID_HYPERPLANES")>>mat_hy){
      const int size=mat_hy.size();
      const int rank=m.give("RANK");
      Matrix<Rational> I(size+2*n_elements,n_elements+1);
      Matrix<Rational> E(1,n_elements+1);
      for (int f=0; f<size; ++f) {
         I(f,0)=rank-1;
         for (Entire< Set<int> >::const_iterator i=entire(mat_hy[f]); !i.at_end(); ++i)
            I(f,(*i)+1)=-1;
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

Function4perl(&matroid_polytope, "matroid_polytope(Matroid)");
Function4perl(&matroid_indices_of_hypersimplex_vertices, "matroid_indices_of_hypersimplex_vertices(Matroid)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
