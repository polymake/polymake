/* Copyright (c) 1997-2023
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
#include "polymake/Rational.h"
#include "polymake/PowerSet.h"
#include "polymake/linalg.h"
#include "polymake/list"


namespace polymake { namespace matroid {

void bases_from_points(BigObject m)
{
   const Matrix<Rational> points=m.give("VECTORS");
   const Int n_elements = points.rows();
   const Int r = rank(points);

   std::list<Set<Int>> bases;
   Int n_bases = 0;

   // test for each subset of size r
   for (auto i=entire(all_subsets_of_k(sequence(0,n_elements),r)); !i.at_end(); ++i) {
      const Matrix<Rational> b=points.minor(*i,All);
      if (rank(b)==r) {
         bases.push_back(*i);
         ++n_bases;
      }
   }

   m.take("BASES") << bases;
   m.take("N_BASES") << n_bases;
   m.take("RANK") << r;
   m.take("N_ELEMENTS") << n_elements;
}

Function4perl(&bases_from_points, "bases_from_points(Matroid)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
