/* Copyright (c) 1997-2022
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
#include "polymake/SparseVector.h"
#include "polymake/SparseMatrix.h"
#include "polymake/GenericMatrix.h"
#include "polymake/ListMatrix.h"
#include "polymake/Matrix.h"

namespace polymake { namespace polytope {

template <typename Scalar, typename MatrixType>
BigObject weighted_digraph_polyhedron(const GenericMatrix<MatrixType, Scalar>& W)
{
   const Int k = W.rows();
   if (W.cols() != k)
         throw std::runtime_error("weighted_digraph_polyhedron: non-square matrix");

   ListMatrix<SparseVector<Scalar>> Ineq;
   for (Int i = 0; i < k; ++i)
      for (Int j = 0; j < k; ++j) {
         if (isfinite(W.top()(i,j)) && (i!=j || W.top()(i,j) != 0)) {
            SparseVector<Scalar> this_ineq(k+1); // initialized as zero vector
            this_ineq[0]=W.top()(i,j);
            if (i!=j) { this_ineq[i+1]=-1; this_ineq[j+1]=1; }
            Ineq /= this_ineq;
         }
      }

   BigObject p_out("Polytope", mlist<Scalar>(), "INEQUALITIES", Ineq);
   p_out.attach("WEIGHTED_DIGRAPH") << Matrix<Scalar>(W);
   p_out.set_description() << "Weighted digraph polyhedron" << endl;
   return p_out;
}

UserFunctionTemplate4perl("# @category Producing a polytope from graphs"
                          "# Weighted digraph polyhedron of a directed graph with a weight function as studied in"
                          "#   Joswig, Loho: Weighted digraph polyhedra and tropical cones, LAA (2016)."
                          "# The graph and the weight function are combined into a matrix."
                          "# @param Matrix encoding weighted digraph"
                          "# @return polytope::Polytope"
                          "# @example This digraph has two nodes and a single arc (with weight 2)."
                          "# > $enc = new Matrix([[0,2],[\"inf\",0]]);"
                          "# > $Q = weighted_digraph_polyhedron($enc);"
                          "# > print $Q->FACETS;"
                          "# | 2 -1 1"
                          "# | 1 0 0"
                          "# These are the one defining inequality and the trivial inequality, which contains the far face.",
                          "weighted_digraph_polyhedron<Scalar>(Matrix<Scalar,_>)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
