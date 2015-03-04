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
#include "polymake/Vector.h"
#include "polymake/Matrix.h"

namespace polymake { namespace tropical {

template <typename Matrix1, typename Matrix2, typename Coord>
Matrix<Coord> multiply(const GenericMatrix<Matrix1,Coord>& A, const GenericMatrix<Matrix2,Coord>& B)
{
   const int m=A.rows(), n=B.cols();
   if (A.cols()!=B.rows())
      throw std::runtime_error("tropical::multiply: dimension mismatch");
   Matrix<Coord> C(m,n);
   for (int i=0; i<m; ++i)
      for (int j=0; j<n; ++j)
         C(i,j)=accumulate(A.row(i)+B.col(j),operations::min());
   return C;
}

template <typename Coord>
Matrix<Coord> dualize(const Matrix<Coord>& points, const Matrix<Coord>& generators)
{
  return multiply(-points,generators);
}

UserFunctionTemplate4perl("# @category Producing a tropical polytope"
                          "# Dualizes a point set with respect to the generators of a tropical polytope."
                          "# The points are dualized with respect to the (rows of the) matrix of the generators."
                          "# Cf."
                          "# \t Develin & Sturmfels, Tropical Convexity, Lemma 22."
                          "# @param Matrix points"
                          "# @param Matrix generators"
                          "# @tparam Coord"
                          "# @return Matrix",
                          "dualize<Coord>(Matrix<Coord> Matrix<Coord>)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
