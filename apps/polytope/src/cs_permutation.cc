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
#include "polymake/Array.h"
#include "polymake/Matrix.h"
#include "polymake/Map.h"

namespace polymake { namespace polytope {

template<typename Scalar>
void cs_permutation(BigObject p)
{
   const Matrix<Scalar> V = p.give("VERTICES");

   Map<Vector<Scalar>, Int> index_of;
   Int index = 0;
   for (auto rit = entire(rows(V)); !rit.at_end(); ++rit)
      index_of[*rit] = index++;

   Array<Int> generator(V.rows());
   auto ait = entire(generator);
   for (auto rit = entire(rows(V)); !rit.at_end(); ++rit) {
      Vector<Scalar> v(-(*rit));
      v[0].negate();
      if (!index_of.contains(v)) {
         p.take("CENTRALLY_SYMMETRIC") << false;
         p.take("CS_PERMUTATION") << Undefined();
         return;
      }
      *ait++ = index_of[v];
   }

   p.take("CENTRALLY_SYMMETRIC") << true;
   p.take("CS_PERMUTATION") << generator;
}

FunctionTemplate4perl("cs_permutation<Scalar>(Polytope<Scalar>)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
