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
#include "polymake/ListMatrix.h"

namespace polymake { namespace polytope {

template <typename Scalar>
Matrix<Scalar>
bounding_box(const Matrix<Scalar>& V, const Scalar& surplus_k, const bool visual)
{
   if (surplus_k<0) throw std::runtime_error("bounding_box: surplus value must be non-negative");
   
   const int d=V.cols();
   int i=V.rows()-1;
   while (i>=0 && is_zero(V(i,0))) --i;
   if (i<0) throw std::runtime_error("bounding_box: no bounded vertices for box.");

   Vector<Scalar> min_vector(V[i].slice(0,d)), max_vector(min_vector);
   for (--i ; i>=0; --i)
      if (!is_zero(V(i,0)))
         for (int j=1; j<d; ++j)
            assign_min_max(min_vector[j], max_vector[j], V(i,j));

   Vector<Scalar> surplus=surplus_k*(max_vector-min_vector);
   // the following is useful for visualization if vertices do not span space
   if (visual) 
      for (int j=1; j<d; ++j)
         if (surplus[j]==0) surplus[j]=1;
   
   max_vector+=surplus;
   min_vector-=surplus;

   ListMatrix< Vector<Scalar> > af(0,d);
   for (int j=1; j<d; ++j) {
      af /= ( max_vector[j] | -unit_vector<Scalar>(d-1,j-1));
      af /= (-min_vector[j] |  unit_vector<Scalar>(d-1,j-1));
   }

   return af;
}

UserFunctionTemplate4perl("# @category Visualization"
                  "# Introduce artificial boundary facets (which are always vertical,"
                  "# i.e., the last coordinate is zero) to allow for bounded images of "
                  "# unbounded polyhedra (e.g. Voronoi polyhedra)."
                  "# If the //voronoi// flag is set, the last direction is left unbounded."
                  "# @param Matrix V vertices that should be in the box"
                  "# @param Scalar surplus_k size of the bounding box relative to the box spanned by //V//"
                  "# @param Bool voronoi useful for visualizations of Voronoi diagrams that do not have enough vertices"
                  "#  default value is 0."
                  "# @return Matrix",
                  "bounding_box<Scalar>(Matrix<Scalar> $; $=0)");

} } 

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
