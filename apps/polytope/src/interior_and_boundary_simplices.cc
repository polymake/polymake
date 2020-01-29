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
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Set.h"
#include "polymake/PowerSet.h"
#include "polymake/linalg.h"
#include "polymake/polytope/simplex_tools.h"
#include "polymake/vector"

namespace polymake { namespace polytope {

typedef Set<Int> SetType;

template <typename Scalar>
Array<SetType> max_interior_simplices_impl(BigObject p, OptionSet options)
{
   const bool is_config = p.isa("PointConfiguration");

   const Int d = p.give(is_config ? Str("CONVEX_HULL.COMBINATORIAL_DIM") : Str("COMBINATORIAL_DIM"));
   const Matrix<Scalar> V = p.give(is_config ? Str("POINTS") : Str("RAYS"));
   const Int n = V.rows();

   AnyString VIF_property = options["VIF_property"];
   if (!VIF_property)
      VIF_property = is_config
                     ? Str("CONVEX_HULL.POINTS_IN_FACETS")
                     : Str("RAYS_IN_FACETS");
   const IncidenceMatrix<> VIF = p.give(VIF_property);

   std::vector<SetType> interior_simplices;
   for (auto fit = entire(all_subsets_of_k(sequence(0,n), d+1)); !fit.at_end(); ++fit) {
       const SetType sigma(*fit);
       if (is_interior(sigma, VIF) && rank(V.minor(sigma, All)) == d+1)
          interior_simplices.push_back(sigma);
   }
   return Array<SetType>{interior_simplices};
}

template <typename Scalar>
std::pair< Array<SetType>, Array<SetType> > interior_and_boundary_ridges(BigObject p, OptionSet options)
{
   const bool is_config = p.isa("PointConfiguration");

   const Int d = p.give(is_config ? Str("CONVEX_HULL.COMBINATORIAL_DIM") : Str("COMBINATORIAL_DIM"));

   AnyString VIF_property = options["VIF_property"];
   if (!VIF_property)
      VIF_property = is_config
                     ? Str("CONVEX_HULL.POINTS_IN_FACETS")
                     : Str("RAYS_IN_FACETS");
   const IncidenceMatrix<> VIF = p.give(VIF_property);

   const Matrix<Scalar> V = p.give(is_config ? Str("POINTS") : Str("RAYS"));

   const Int n = V.rows();

   std::vector<SetType> interior_ridges, boundary_ridges;
   for (auto fit = entire(all_subsets_of_k(sequence(0,n), d)); !fit.at_end(); ++fit) {
       const SetType sigma(*fit);
       if (rank(V.minor(sigma,All)) < d) continue;
       if (is_in_boundary(sigma, VIF))
          boundary_ridges.push_back(sigma);
       else
          interior_ridges.push_back(sigma);
   }
   return { Array<SetType>{interior_ridges}, Array<SetType>{boundary_ridges} };
}

FunctionTemplate4perl("max_interior_simplices_impl<Scalar=Rational>($ { VIF_property => undef })");

UserFunctionTemplate4perl("# @category Triangulations, subdivisions and volume"
                          "# Find the (//d//-1)-dimensional simplices in the interior and in the boundary of a //d//-dimensional polytope or cone"
                          "# @param Polytope P the input polytope or cone"
                          "# @return Pair<Array<Set>,Array<Set>>"
                          "# @example"
                          "# > print interior_and_boundary_ridges(cube(2));"
                          "# | <{0 3}"
                          "# | {1 2}"
                          "# | >"
                          "# | <{0 1}"
                          "# | {0 2}"
                          "# | {1 3}"
                          "# | {2 3}"
                          "# | >",
                          "interior_and_boundary_ridges<Scalar=Rational>($ { VIF_property=>undef })");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:

