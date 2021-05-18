/* Copyright (c) 1997-2021
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
#include "polymake/Set.h"
#include "polymake/Matrix.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Array.h"

namespace polymake { namespace polytope {
 
template<typename Scalar, typename MatrixType, typename VectorType>
Array<Set<Int>>
regular_subdivision(const GenericMatrix<MatrixType> &vertices_in, const GenericVector<VectorType>& weight_in)
{
   auto vertices = convert_to<Scalar>(vertices_in);
   auto weight = convert_to<Scalar>(weight_in);
   //construct the lifted polytope + a ray
   const Matrix<Scalar> lifted_vertices = (vertices | weight) / unit_vector<Scalar>(vertices.cols()+1, vertices.cols());
   BigObject p("Polytope", mlist<Scalar>(), "POINTS", lifted_vertices);

   //we have to check for the case in which the subdivision is trivial
   const Matrix<Scalar> aff_hull = p.give("AFFINE_HULL");
   if (aff_hull.rows() && !is_zero(cols(aff_hull).back())) {
      const Set<Int> all = sequence(0,vertices.rows());
      return Array<Set<Int>>(1, all);
   }
    
   const Matrix<Scalar> facets = p.give("FACETS");
   const IncidenceMatrix<> vif = p.give("POINTS_IN_FACETS");
    
   Set<Int> simplices;
   Int i = 0;
   for (auto last_col = entire(cols(facets).back()); !last_col.at_end(); ++last_col, ++i)
      if (*last_col>0)
         simplices.push_back(i); //the lower facets are those with last coordinate>0

   return Array<Set<Int>>(select(rows(vif), simplices));
}

UserFunctionTemplate4perl("# @category Triangulations, subdivisions and volume"
                          "# Compute a regular subdivision of the polytope obtained"
                          "# by lifting //points// to //weights// and taking the lower"
                          "# complex of the resulting polytope."
                          "# If the weight is generic the output is a triangulation."
                          "# @param Matrix points"
                          "# @param Vector weights"
                          "# @return Array<Set<Int>>"
                          "# @example [prefer cdd] [require bundled:cdd] The following generates a regular subdivision of the square."
                          "# > $w = new Vector(2,23,2,2);"
                          "# > $r = regular_subdivision(cube(2)->VERTICES,$w);"
                          "# > print $r;"
                          "# | {0 2 3}"
                          "# | {0 1 3}"
                          "# @author Sven Herrmann",
                          "regular_subdivision<Scalar> [ is_ordered_field(type_upgrade<Scalar, Rational>) ](Matrix<type_upgrade<Scalar>> Vector<type_upgrade<Scalar>>)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
