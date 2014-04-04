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

namespace polymake { namespace matroid {

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

  p.take("VERTICES") << V;
  p.take("CONE_AMBIENT_DIM") << n_elements+1;
  
  m.take("POLYTOPE") << p;
}

Function4perl(&matroid_polytope, "matroid_polytope(Matroid)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
