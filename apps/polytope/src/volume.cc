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
#include "polymake/Integer.h"
#include "polymake/Array.h"
#include "polymake/Matrix.h"
#include "polymake/linalg.h"

namespace polymake { namespace polytope {

template <typename Matrix, typename Scalar, typename Triangulation>
Scalar volume(const GenericMatrix<Matrix, Scalar>& Points, const Triangulation& tr)
{
   Scalar V(0);
   const Int dim = tr.front().size()-1;
   for (auto s=entire(tr); !s.at_end(); ++s)
      V += abs(det( Points.minor(*s,All) ));
   return V / Integer::fac(dim);
}

template <typename MatrixType, typename Scalar, typename Triangulation>
Array<Scalar> squared_relative_volumes(const GenericMatrix<MatrixType, Scalar>& Points, const Triangulation& tr)
{
   Array<Scalar> V(tr.size());
   auto vit = entire(V);
   const Int dim = tr.front().size()-1;
   const Integer f = Integer::fac(dim);
   for (auto s=entire(tr); !s.at_end(); ++s, ++vit) {
      Matrix<Scalar> M(Points.minor(*s, All));
      // subtract off the first row from all the others, to place one vertex at the origin
      for (Int i = 1; i < M.rows(); ++i) 
         M.row(i) -= M.row(0);
      *vit = abs(det( M.minor(range_from(1), All) * T(M.minor(range_from(1), All)) )) / (f*f);
   }
   return V;
}

FunctionTemplate4perl("volume(Matrix *)");

FunctionTemplate4perl("squared_relative_volumes(Matrix *)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
