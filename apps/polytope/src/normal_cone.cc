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
#include "polymake/Set.h"
#include "polymake/Matrix.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Graph.h"

namespace polymake { namespace polytope  {

template <typename Scalar>
BigObject normal_cone_impl(BigObject p,
                           const Set<Int>& F,
                           const std::string& ftv_section,
                           const std::string& rays_section,
                           const std::string& facets_section,
                           OptionSet options)
{
   if (p.isa("Polytope")) {
      const Set<Int> far_face = p.give("FAR_FACE");
      if (incl(F, far_face) <= 0)
         throw std::runtime_error("normal_cone: face is contained in the far face");
   }
   const bool
      outer  = options["outer"],
      attach = options["attach"];

   const IncidenceMatrix<> ftv = p.give(ftv_section);
   const Matrix<Scalar> facet_normals = p.give(facets_section);
   Matrix<Scalar> cone_normals(facet_normals.minor(accumulate(rows(ftv.minor(F,All)), operations::mul()), range_from(1)));
   if (outer) cone_normals = -cone_normals;

   BigObject c("Cone", mlist<Scalar>());
   const Matrix<Scalar> ls = p.give("LINEAR_SPAN");
   if (attach) {
      const Matrix<Scalar> rays = p.give(rays_section);
      c.take("INPUT_RAYS") << rays.minor(F,All) / ( zero_vector<Scalar>() | cone_normals );
      c.take("INPUT_LINEALITY") << ls;
      c.take("CONE_AMBIENT_DIM") << cone_normals.cols()+1;
   } else {
      c.take("INPUT_RAYS") << cone_normals;
      c.take("INPUT_LINEALITY") << ls.minor(All, range_from(1));
      c.take("CONE_AMBIENT_DIM") << cone_normals.cols();
   }
   return c;
}

template <typename Scalar>
BigObject inner_cone_impl(BigObject p,
                             const Set<Int>& F,
                             OptionSet options)
{
   if (p.isa("Polytope")) {
      const Set<Int> far_face = p.give("FAR_FACE");
      if (incl(F, far_face) <= 0)
         throw std::runtime_error("normal_cone: face is contained in the far face");
   }
   const bool
      outer  = options["outer"],
      attach = options["attach"];

   const Graph<> G = p.give("GRAPH.ADJACENCY");
   const Matrix<Scalar> V = p.give("VERTICES");
   std::vector<Vector<Scalar>> inner_rays_list;
   for (Int v : F) {
      for (Int w : G.out_adjacent_nodes(v) - F) {
         if (outer) {
            inner_rays_list.emplace_back(V[v] - V[w]);
         } else {
            inner_rays_list.emplace_back(V[w] - V[v]);
         }
      }
   }
   Matrix<Scalar> inner_rays(inner_rays_list.size(), V.cols(), entire(inner_rays_list));
   inner_rays.col(0) = zero_vector<Scalar>(inner_rays.rows());

   BigObject c("Cone", mlist<Scalar>());
   const Matrix<Scalar> ls = p.give("LINEAR_SPAN");
   if (attach) {
      const Matrix<Scalar> rays = p.give("RAYS");
      c.take("INPUT_RAYS") << rays.minor(F,All) / inner_rays;
      c.take("INPUT_LINEALITY") << ls;
      c.take("CONE_AMBIENT_DIM") << V.cols();
   } else {
      c.take("INPUT_RAYS") << inner_rays.minor(All, range_from(1));
      c.take("INPUT_LINEALITY") << ls.minor(All, range_from(1));
      c.take("CONE_AMBIENT_DIM") << V.cols()-1;
   }
   return c;
}

FunctionTemplate4perl("normal_cone_impl<Scalar>($$$$$$)");

FunctionTemplate4perl("inner_cone_impl<Scalar>($$$)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
