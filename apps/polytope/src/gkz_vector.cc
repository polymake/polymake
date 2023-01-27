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
#include "polymake/GenericMatrix.h"
#include "polymake/Array.h"
#include "polymake/Set.h"
#include "polymake/linalg.h"
#include "polymake/QuadraticExtension.h"
#include "polymake/PowerSet.h"

namespace polymake { namespace polytope {

template <typename Scalar, typename Container, typename MatrixTop>
Vector<Scalar> gkz_vector(const GenericMatrix<MatrixTop,Scalar>& vert, const Container& triang)
{
   Vector<Scalar> gkz(vert.top().rows(),0);

   // go through all simplices
   for (const auto& simplex: triang) {
      const Scalar v=abs(det(vert.top().minor(simplex,All)));
      for (const Int j : simplex)
         gkz[j]+=v;
   }

   return gkz;
}


FunctionTemplate4perl("gkz_vector<Scalar,Container>(Matrix<Scalar>, Container)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
