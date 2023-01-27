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
#include "polymake/Graph.h"

namespace polymake { namespace polytope {

template<typename Scalar>
Graph<> split_compatibility_graph(const Matrix<Scalar>& SplitEquations, BigObject p)
{
   const Matrix<Scalar> Facets = p.give("FACETS");
   const Matrix<Scalar> AffineHull = p.give("AFFINE_HULL");
   const Int n_splits = SplitEquations.rows();

   Graph<> S(n_splits);
   BigObjectType Polytope("Polytope", mlist<Scalar>());

   for (Int s1 = 0; s1 < n_splits; ++s1) {
      for (Int s2 = s1+1; s2 < n_splits; ++s2) {
         Matrix<Scalar> SplitIntersection = vector2row(SplitEquations.row(s1)) / SplitEquations.row(s2) / AffineHull;
         BigObject Intersection(Polytope,
                                "INEQUALITIES", Facets,
                                "EQUATIONS", SplitIntersection);
         const bool InterSectionNonEmpty = Intersection.give("FEASIBLE");
         if (InterSectionNonEmpty) {
            Vector<Scalar> IntersectionPoint = Intersection.give("REL_INT_POINT");
            for (auto f = entire(rows(Facets)); !f.at_end();  ++f)
               if ((*f) * IntersectionPoint == 0) {
                  S.edge(s1, s2);
                  break;
               }
         } else {
            S.edge(s1,s2);
         }
      }
   }

   return S;
}

UserFunctionTemplate4perl("# @category Triangulations, subdivisions and volume"
                          "#DOC_FIXME: Incomprehensible description!"
                          "# Computes the compatibility graph among the //splits// of a polytope //P//."
                          "# @param Matrix splits the splits given by split equations"
                          "# @param Polytope P the input polytope"
                          "# @return Graph",
                          "split_compatibility_graph<Scalar>(Matrix<type_upgrade<Scalar>> Polytope<type_upgrade<Scalar>>)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
