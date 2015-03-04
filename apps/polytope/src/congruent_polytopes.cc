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
#include "polymake/graph/compare.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/Graph.h"

namespace polymake { namespace polytope {
namespace {

template <typename Scalar> inline
void fill_distances(int nv, const Matrix<Scalar>& M, Graph<>& G, Vector<Scalar>& dist)
{
   for (int i=0, n=nv-1; i<n; ++i)
      for (int j=i+1; j<=n; ++j, ++nv) {
         dist[nv]=sqr(M[i]-M[j]);
         G.edge(nv,i);
         G.edge(nv,j);
      }
}
}

template <typename Scalar>
Scalar congruent(perl::Object p1, perl::Object p2)
{
   const Matrix<Scalar> V1=p1.give("VERTICES"), V2=p2.give("VERTICES");
   const int nv=V1.rows();
   if (nv != V2.rows() || V1.cols() != V2.cols()) return 0;

   // original vertices + edge between each two vertices
   const int n_nodes=nv*(nv+1)/2;
   Graph<> G1(n_nodes), G2(n_nodes);
   Vector<Scalar> dist1(n_nodes), dist2(n_nodes);
   fill_distances(nv, V1, G1, dist1);
   fill_distances(nv, V2, G2, dist2);

   Scalar min1=accumulate(dist1.slice(nv), operations::min()),
      min2=accumulate(dist2.slice(nv), operations::min());

   min1 /= min2;
   if (min1 != 1) dist2.slice(nv)*=min1;
   
   return graph::isomorphic(G1, dist1, G2, dist2) ? min1 : Scalar(0);
}

UserFunctionTemplate4perl("# @category Comparing"
                          "# Check whether two given polytopes //P1// and //P2// are congruent, i.e. whether"
                          "# there is an affine isomorphism between them that is induced by a (possibly scaled) orthogonal matrix."
                          "# Returns the scale factor, or 0 if the polytopes are not congruent."
                          "# "
                          "# We are using the reduction of the congruence problem (for arbitrary point sets) to the graph"
                          "# isomorphism problem due to:"
                          "#\t Akutsu, T.: On determining the congruence of point sets in `d` dimensions."
                          "#\t Comput. Geom. Theory Appl. 9, 247--256 (1998), no. 4"
                          "# @param Polytope P1 the first polytope"
                          "# @param Polytope P2 the second polytope"
                          "# @return Scalar the scale factor or 0 if the polytopes are not congruent"
                          "# @author Alexander Schwartz",
                          "congruent<Scalar> (Polytope<Scalar>, Polytope<Scalar>)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
