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
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Set.h"
#include "polymake/PowerSet.h"
#include "polymake/linalg.h"
#include "polymake/polytope/simplex_tools.h"

namespace polymake { namespace polytope {

typedef Set<int> SetType;

template <typename Scalar>
Array<SetType> max_interior_simplices_impl(perl::Object p, perl::OptionSet options)
{
   const bool is_config = p.isa("PointConfiguration");

   const int d = is_config 
      ? p.give("DIM")
      : p.give("COMBINATORIAL_DIM");

   const std::string points = is_config
      ? std::string("POINTS")
      : std::string("RAYS");
   const Matrix<Scalar> V = p.give(points.c_str()); 
   const int n = V.rows();
   
   std::string vif_property = options["vif_property"];
   if (!vif_property.size()) vif_property = is_config
                                ? std::string("CONVEX_HULL.VERTICES_IN_FACETS")
                                : std::string("RAYS_IN_FACETS");
   const IncidenceMatrix<> VIF = p.give(vif_property.c_str());

   Set<SetType> interior_simplices;
   for (Entire<Subsets_of_k<const sequence&> >::const_iterator fit = entire(all_subsets_of_k(sequence(0,n), d+1)); !fit.at_end(); ++fit) {
       const SetType sigma(*fit);
       if (is_interior(sigma, VIF) && rank(V.minor(sigma, All)) == d+1)
           interior_simplices += sigma;
   }
   return interior_simplices;
}

template <typename Scalar>
std::pair< Array<SetType>, Array<SetType> > interior_and_boundary_ridges(perl::Object p, perl::OptionSet options)
{
   const bool is_config = p.isa("PointConfiguration");

   const int d = is_config 
      ? p.give("DIM")
      : p.give("COMBINATORIAL_DIM");

   std::string vif_property = options["vif_property"];
   if (!vif_property.size()) vif_property = is_config
                                ? std::string("CONVEX_HULL.VERTICES_IN_FACETS")
                                : std::string("RAYS_IN_FACETS");
   const IncidenceMatrix<> VIF = p.give(vif_property.c_str());

   const Matrix<Scalar> V = is_config
      ? p.give("POINTS")
      : p.give("RAYS");

   const int n = V.rows();

   Set<SetType> interior_ridges, boundary_ridges;
   for (Entire<Subsets_of_k<const sequence&> >::const_iterator fit = entire(all_subsets_of_k(sequence(0,n), d)); !fit.at_end(); ++fit) {
       const SetType sigma(*fit);
       if (rank(V.minor(sigma,All)) < d-1) continue;
       if (is_in_boundary(sigma, VIF)) boundary_ridges += sigma;
       else interior_ridges += sigma;
   }
   return std::make_pair<Array<SetType>, Array<SetType> >(interior_ridges, boundary_ridges);
}

UserFunctionTemplate4perl("# @category Triangulations, subdivisions and volume"
                          "# Find the interior //d//-dimensional simplices of a polytope or cone of combinatorial dimension //d//"
                          "# @param Polytope P the input polytope or cone"
                          "# @return Array<Set>",
                          "max_interior_simplices_impl<Scalar=Rational>($ { VIF_property=>'CONVEX_HULL.VERTICES_IN_FACETS' })");

UserFunctionTemplate4perl("# @category Triangulations, subdivisions and volume"
                          "# Find the (//d//-1)-dimensional simplices in the interior and in the boundary of a //d//-dimensional polytope or cone"
                          "# @param Polytope P the input polytope or cone"
                          "# @return Pair<Array<Set>,Array<Set>>",
                          "interior_and_boundary_ridges<Scalar=Rational>($ { VIF_property=>'CONVEX_HULL.VERTICES_IN_FACETS' })");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:

