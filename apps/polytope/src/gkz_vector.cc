/* Copyright (c) 1997-2018
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
#include "polymake/GenericMatrix.h"
#include "polymake/Array.h"
#include "polymake/Set.h"
#include "polymake/linalg.h"

namespace polymake { namespace polytope {

template <typename Scalar, typename MatrixTop>
Vector<Scalar> gkz_vector(const GenericMatrix<MatrixTop,Scalar>& vert, const Array<Set<int>>& triang)
{
   Vector<Scalar> gkz(vert.top().rows(),0);

   // go through all simplices
   for (auto i=entire(triang); !i.at_end(); ++i) {
      const Scalar v=abs(det(vert.top().minor(*i,All)));
      for (auto j=entire(*i); !j.at_end(); ++j)
         gkz[(*j)]+=v;
   }

   return gkz;
}

FunctionTemplate4perl("gkz_vector<Scalar>(Matrix<Scalar>,Array<Set>)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
