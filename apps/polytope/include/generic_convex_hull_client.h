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

#pragma once

#include "polymake/client.h"
#include "polymake/polytope/convex_hull.h"

namespace polymake { namespace polytope {


template <typename Scalar, typename Solver>
void generic_convex_hull_primal(BigObject& p, bool isCone, const Solver& solver)
{
   Matrix<Scalar> Points = p.give("RAYS | INPUT_RAYS"),
               Lineality = p.lookup("LINEALITY_SPACE | INPUT_LINEALITY");

   const auto sol = enumerate_facets(Points, Lineality, isCone, solver);
   p.take("FACETS") << sol.first;
   if (isCone) {
      p.take("LINEAR_SPAN") << sol.second;
   } else {
      p.take("AFFINE_HULL") << sol.second;
   }
}

template <typename Scalar, typename Solver>
void generic_convex_hull_dual(BigObject& p, bool isCone, const Solver& solver)
{
   Matrix<Scalar> H = p.give("FACETS | INEQUALITIES"),
                 EQ = p.lookup("LINEAR_SPAN | EQUATIONS");


   // * we handle the case of polytopes with empty exterior description somewhat special:
   //    empty facet matrix implies that the polytope must be empty!
   //    empty inequalities cannot occur due to the far face initial rule
   // * this also covers the case when the ambient dimension is empty for polytopes
   // * for cones an empty exterior description describes the whole space and is handled
   //   correctly in the interfaces
   if (isCone || H.rows() > 0 || EQ.rows() > 0) {
      try {
         const auto sol = enumerate_vertices(H, EQ, isCone, solver);
         p.take("RAYS") << sol.first;
         if (isCone) {
            p.take("LINEALITY_SPACE") << sol.second;
         } else {
            p.take("RAYS") << sol.first;
            p.take("LINEALITY_SPACE") << sol.second;
         }
         p.take("POINTED") << (sol.second.rows()==0);
         p.take("LINEALITY_DIM") << sol.second.rows();
         return;
      }
      catch (const infeasible&) { }
   }
   const Int d = H.cols();
   p.take("RAYS") << Matrix<Scalar>(0, d);
   p.take("LINEALITY_SPACE") << Matrix<Scalar>(0, d);
   p.take("LINEALITY_DIM") << 0;
   p.take("POINTED") << true;
}

} }


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
