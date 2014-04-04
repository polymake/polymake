/* Copyright (c) 1997-2014
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
#include "polymake/Set.h"
#include "polymake/Matrix.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Rational.h"
#include "polymake/Array.h"

namespace polymake { namespace polytope {
 
Array< Set <int> >
regular_subdivision(const Matrix<Rational> &vertices, const Vector<Rational>& weight)
{
   //construct the lifted polytope
   const Matrix<Rational> lifted_vertices=vertices|weight;
   perl::Object p("Polytope<Rational>");
   p.take("POINTS") << lifted_vertices;

   //we have to check for the case in which the subdivision is trivial
   const Matrix<Rational> aff_hull = p.give("AFFINE_HULL");
   if (aff_hull.rows() && !is_zero(cols(aff_hull).back())) {
      const Set<int> all=sequence(0,vertices.rows());
      return Array<Set <int> >(1,all);
   }
    
   const Matrix<Rational> facets=p.give("FACETS");
   const IncidenceMatrix<> vif=p.give("POINTS_IN_FACETS");
    
   Set<int> simplices;
   int i=0;
   for (Entire< Matrix<Rational>::col_type >::const_iterator last_col=entire(cols(facets).back());
        !last_col.at_end(); ++last_col, ++i)
      if (*last_col>0)
         simplices.push_back(i); //the lower facets are those with last coordinate>0

   return Array< Set<int> >(select(rows(vif), simplices));
}

UserFunction4perl("# @category Triangulations, subdivisions and volume"
                  "# Compute a regular subdivision of the polytope obtained"
                  "# by lifting //points// to //weights// and taking the lower"
                  "# complex of the resulting polytope."
                  "# If the weight is generic the output is a triangulation."
                  "# @param Matrix points"
                  "# @param Vector weights"
                  "# @return Array<Set<Int>>"
                  "# @author Sven Herrmann",
                  &regular_subdivision,"regular_subdivision");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
