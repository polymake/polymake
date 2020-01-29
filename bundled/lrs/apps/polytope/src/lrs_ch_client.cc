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
#include "polymake/polytope/lrs_interface.h"
#include "polymake/polytope/generic_convex_hull_client.h"

namespace polymake { namespace polytope { namespace lrs_interface {

template <typename Scalar=Rational>
auto create_convex_hull_solver()
{
   return cached_convex_hull_solver<Rational>(new ConvexHullSolver(), true);
}

}

void lrs_ch_primal(BigObject p, const bool verbose, const bool isCone)
{
   generic_convex_hull_primal<Rational>(p, isCone, lrs_interface::ConvexHullSolver(verbose));
}

void lrs_ch_dual(BigObject p, const bool verbose, const bool isCone)
{
   generic_convex_hull_dual<Rational>(p, isCone, lrs_interface::ConvexHullSolver(verbose));
}

void lrs_count_vertices(BigObject p, const bool only_bounded, const bool verbose, const bool isCone)
{
   const lrs_interface::ConvexHullSolver solver(verbose);
   Matrix<Rational> H = p.give("FACETS | INEQUALITIES"),
                   EQ = p.lookup("LINEAR_SPAN | EQUATIONS");

   if (!align_matrix_column_dim(H, EQ, isCone))
      throw std::runtime_error("count_vertices - dimension mismatch between FACETS|INEQUALITIES and LINEAR_SPAN|EQUATIONS");

   // * we handle the case of polytopes with empty exterior description somewhat special:
   //    empty facet matrix implies that the polytope must be empty!
   //    empty inequalities cannot occur due to the far face initial rule
   // * this also covers the case when the ambient dimension is empty for polytopes
   // * for cones an empty exterior description describes the whole space and is handled
   //   correctly in the interfaces
   if (isCone || H.rows() > 0 || EQ.rows() > 0) {
      if (isCone && only_bounded)
         throw std::runtime_error("a cone has no bounded vertices");

      try {
         const auto count = solver.count_vertices(H, EQ, only_bounded);
         if (isCone) {
            // lrs counts the origin
            // we have to substract this in our representation
            p.take("N_RAYS") << count.n_vertices-1;
         } else {
            if (!only_bounded) p.take("N_VERTICES") << count.n_vertices;
            p.take("N_BOUNDED_VERTICES") << count.n_bounded_vertices;
         }
         p.take("POINTED") << (count.lineality_dim ==0 );
         p.take("LINEALITY_DIM") << count.lineality_dim;
         return;
      }
      catch (const infeasible&) { }
   }
   p.take("POINTED") << true;
   p.take("LINEALITY_DIM") << 0;
   if (!only_bounded)
      p.take("N_RAYS") << 0;
   if (!isCone)
      p.take("N_BOUNDED_VERTICES") << 0;
}

void lrs_count_facets(BigObject p, const bool verbose, const bool isCone)
{
   const lrs_interface::ConvexHullSolver solver(verbose);
   Matrix<Rational> Points = p.give("RAYS | INPUT_RAYS"),
                 Lineality = p.lookup("LINEALITY_SPACE | INPUT_LINEALITY");

   if (!align_matrix_column_dim(Points, Lineality, isCone))
      throw std::runtime_error("count_facets - dimension mismatch between RAYS|INPUT_RAYS and LINEALITY_SPACE|INPUT_LINEALITY");

   p.take("N_FACETS") << solver.count_facets(Points, Lineality, isCone);
}

Function4perl(&lrs_ch_primal, "lrs_ch_primal(Cone<Rational>; $=false, $=true)");
Function4perl(&lrs_ch_dual, "lrs_ch_dual(Cone<Rational>; $=false, $=true)");

Function4perl(&lrs_ch_primal, "lrs_ch_primal(Polytope<Rational>; $=false, $=false)");
Function4perl(&lrs_ch_dual, "lrs_ch_dual(Polytope<Rational>; $=false, $=false)");

Function4perl(&lrs_count_vertices, "lrs_count_vertices(Cone<Rational>, $; $=false, $=true)");
Function4perl(&lrs_count_vertices, "lrs_count_vertices(Polytope<Rational>, $; $=false, $=false)");

Function4perl(&lrs_count_facets, "lrs_count_facets(Cone<Rational>; $=false, $=true)");
Function4perl(&lrs_count_facets, "lrs_count_facets(Polytope<Rational>; $=false, $=false)");

InsertEmbeddedRule("function lrs.convex_hull: create_convex_hull_solver<Scalar> [Scalar==Rational] ()"
                   " : c++ (name => 'lrs_interface::create_convex_hull_solver') : returns(cached);\n");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
