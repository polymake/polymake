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
#include "polymake/Matrix.h"
#include "polymake/Array.h"
#include "polymake/Rational.h"
#include "polymake/Set.h"

namespace polymake { namespace matroid {

Array< Set <int> > bases_from_matroid_polytope(const Matrix<Rational>& verts)
{
  Array<Set<int>> bases(verts.rows());

  for (int i=0;i<verts.rows();++i) {
    Set<int> b;
    for (int j=1;j<verts.cols();++j)
      if (verts(i,j)!=0) b.insert(j-1);
    bases[i]=b;
  }

  return bases;
}


perl::Object matroid_from_matroid_polytope(perl::Object p)
{
  perl::Object m("Matroid");
  m.take("BASES") << bases_from_matroid_polytope(p.give("VERTICES"));
  const int n_elements = p.call_method("AMBIENT_DIM");
  m.take("N_ELEMENTS") << n_elements;
  m.take("POLYTOPE") << p;

  return m;
}

Function4perl(&bases_from_matroid_polytope, "bases_from_matroid_polytope");
UserFunction4perl("# @category Producing a matroid from other objects\n"
                  "# Creates a matroid from the corresponding matroid\n"
                  "# polytope //p//.\n"
                  "# @param polytope::Polytope p"
                  "# @return Matroid",
                  &matroid_from_matroid_polytope, "matroid_from_matroid_polytope(polytope::Polytope)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
