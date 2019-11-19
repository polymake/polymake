/* Copyright (c) 1997-2019
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
#include "polymake/polytope/lrs_interface.h"
#include "polymake/polytope/generic_convex_hull_client.h"

namespace polymake { namespace polytope {

void lrs_get_non_redundant_points(perl::Object p, const bool isCone)
{
   lrs_interface::ConvexHullSolver solver;
   Matrix<Rational> P = p.give("INPUT_RAYS"),
                    L = p.give("LINEALITY_SPACE");

   if (!align_matrix_column_dim(P, L, isCone))
      throw std::runtime_error("lrs_get_non_redundant_points - dimension mismatch between input properties");

   const auto V = solver.find_irredundant_representation(P, L, false);
   if (isCone) {
     p.take("RAYS") << P.minor(V.first, range_from(1));
     p.take("LINEAR_SPAN") << V.second.minor(range_from(1), range_from(1));
   } else {
     p.take("RAYS") << P.minor(V.first,All);
     p.take("LINEAR_SPAN") << V.second;
   }
   p.take("POINTED") << (L.rows() == 0);
}

void lrs_get_non_redundant_inequalities(perl::Object p, const bool isCone)
{
   lrs_interface::ConvexHullSolver solver;
   Matrix<Rational> P = p.give("INEQUALITIES"),
                    L = p.give("LINEAR_SPAN");

   if (!align_matrix_column_dim(P, L, isCone))
      throw std::runtime_error("lrs_get_non_redundant_inequalities - dimension mismatch between input properties");

   const auto V = solver.find_irredundant_representation(P, L, true);
   if (isCone) {
     p.take("FACETS") << P.minor(V.first, range_from(1));
     p.take("LINEALITY_SPACE") << V.second.minor(All, range_from(1));
   } else {
     p.take("FACETS") << P.minor(V.first,All);
     p.take("LINEALITY_SPACE") << V.second;
   }
}

Function4perl(&lrs_get_non_redundant_points, "lrs_get_non_redundant_points(Cone<Rational>; $=true)");
Function4perl(&lrs_get_non_redundant_points, "lrs_get_non_redundant_points(Polytope<Rational>; $=false)");
Function4perl(&lrs_get_non_redundant_inequalities, "lrs_get_non_redundant_inequalities(Cone<Rational>; $=true)");
Function4perl(&lrs_get_non_redundant_inequalities, "lrs_get_non_redundant_inequalities(Polytope<Rational>; $=false)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
