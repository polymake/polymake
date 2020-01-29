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
#include "polymake/Rational.h"
#include "polymake/Array.h"
#include "polymake/Set.h"
#include "polymake/Matrix.h"
#include "polymake/Integer.h"
#include "polymake/graph/Lattice.h"

namespace polymake { namespace polytope {

void matroid_polytope(BigObject m,  OptionSet options)
{
  const Array<Set<Int>> bases = m.give("BASES");
  const Int n_bases = bases.size();
  const Int n_elements = m.give("N_ELEMENTS");
  
  Matrix<Rational> V(n_bases, n_elements+1);
  
  //test for each subset of size r
  for (Int b = 0; b < n_bases; ++b) {
    V(b,0) = 1;
    for (auto i = entire(bases[b]); !i.at_end(); ++i)
      V(b, (*i)+1) = 1;
  }

  bool ineq_flag = options["inequalities"];
  if (ineq_flag && m.give("CONNECTED") && n_elements>1) {
     BigObject lattice_obj = m.give("LATTICE_OF_FLATS");
     graph::Lattice<graph::lattice::BasicDecoration, graph::lattice::Sequential> lattice(lattice_obj);
     const Int size = lattice.nodes()-2; // do not use the bottom and top node
     const Int rank = m.give("RANK");
     Matrix<Rational> I(size+2*n_elements,n_elements+1);
     Matrix<Rational> E(1,n_elements+1);
     Int f = 0;
     for (Int j = 1; j < rank; ++j) {
        for (auto fi=entire(lattice.nodes_of_rank(j)); !fi.at_end(); ++fi, ++f) {
           I(f,0) = j;
           for (auto i = entire(lattice.face(*fi)); !i.at_end(); ++i)
              I(f, (*i)+1) = -1;
        }
     }
     // hypersimplex
     // 0 <= x_i <= 1 + sum x_i = rank :
     E(0,0) = -rank;
     for (Int i = 0; i < n_elements; ++i) {
        I(size+2*i,i+1)=1;
        I(size+2*i+1,0)=1;
        I(size+2*i+1,i+1)=-1;
        E(0,i+1)=1;
     }
     m.take("POLYTOPE.INEQUALITIES") << I;
     m.take("POLYTOPE.EQUATIONS") << E;
  }

  m.take("POLYTOPE.VERTICES") << V;
  m.take("POLYTOPE.CONE_AMBIENT_DIM") << n_elements+1;
  m.take("POLYTOPE.FEASIBLE") << (bases.size() > 0);
  m.take("POLYTOPE.BOUNDED") << 1;
}

InsertEmbeddedRule("REQUIRE_APPLICATION matroid\n\n");

Function4perl(&matroid_polytope, "matroid_polytope(matroid::Matroid, { inequalities => undef })");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
