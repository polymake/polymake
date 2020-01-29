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
#include "polymake/Matrix.h"
#include "polymake/Set.h"
#include "polymake/Graph.h"
#include "polymake/Vector.h"
#include "polymake/Array.h"
#include "polymake/color.h"

namespace polymake { namespace polytope {

Array<RGB> vertex_colors(BigObject p, BigObject lp, OptionSet options)
{
   const Set<Int> max_face = lp.give("MAXIMAL_FACE"),
                  min_face = lp.give("MINIMAL_FACE");
   Set<Int> rays;
   Vector<double> obj;

   if (lp.lookup("LINEAR_OBJECTIVE", allow_conversion()) >> obj) {
      const Matrix<double> V=p.give("VERTICES");
      obj=V*obj;
      p.give("FAR_FACE") >> rays;
   } else {
      lp.give("ABSTRACT_OBJECTIVE", allow_conversion()) >> obj;
   }

   double max_val=lp.give("MAXIMAL_VALUE"),
          min_val=lp.give("MINIMAL_VALUE");
   const bool upper_bound=isfinite(max_val),
              lower_bound=isfinite(min_val);

   if (!upper_bound || !lower_bound) {
      const Graph<> G=p.give("GRAPH.ADJACENCY");
      const Set<Int> neighbors_of_far = accumulate(select(rows(adjacency_matrix(G)),rays), operations::add())-rays;
      if (!upper_bound)
         max_val = accumulate(obj.slice(neighbors_of_far), operations::max());
      if (!lower_bound)
         min_val = accumulate(obj.slice(neighbors_of_far), operations::min());
   }

   double diff=max_val-min_val;
   if (!upper_bound) max_val+=0.1*diff;
   if (!lower_bound) min_val-=0.1*diff;
   diff=max_val-min_val;

   const Int n_vertices = obj.size();
   const RGB white(1.,1.,1.);
   Array<RGB> VC(n_vertices, white);

   if (diff != 0) {
      RGB max_rgb(white), min_rgb;
      options["max"] >> max_rgb;
      options["min"] >> min_rgb;
      HSV maxcolor(max_rgb), mincolor(min_rgb);

      // If the maximal (minimal) face is affine, it is fully saturated,
      // otherwise the points towards the unbounded direction are made pale.
      if (!upper_bound) maxcolor.saturation *= 0.5;
      if (!lower_bound) mincolor.saturation *= 0.5;

      const double Hmin = mincolor.hue,
                   Hmax = maxcolor.hue,
                  Hdiff = Hmax-Hmin,
                   Smin = mincolor.saturation,
                   Smax = maxcolor.saturation,
                  Sdiff = Smax-Smin,
                   Vmin = mincolor.value,
                   Vmax = maxcolor.value,
                  Vdiff = Vmax-Vmin;

      auto c = VC.begin();
      for (Int v = 0; v < n_vertices; ++v, ++c) {
         if (rays.contains(v)) {
            if (max_face.contains(v))
               *c = maxcolor;
            else if (min_face.contains(v))
               *c = mincolor;
         } else {
            const double k = (max_val-obj[v])/diff;
            *c = HSV(Hmax-k*Hdiff, Smax-k*Sdiff, Vmax-k*Vdiff);
         }
      }
   }
   return VC;
}

UserFunction4perl("# @category Optimization"
                  "# Calculate RGB-color-values for each vertex depending on a linear or abstract objective function."
                  "# Maximal and minimal affine vertices are colored as specified.  Far vertices (= rays) orthogonal"
                  "# to the linear function normal vector are white.  The colors for other affine vertices"
                  "# are linearly interpolated in the HSV color model."
                  "# "
                  "# If the objective function is linear and the corresponding LP problem is unbounded, then"
                  "# the affine vertices that would become optimal after the removal of the rays are painted pale."
                  "# @param Polytope P"
                  "# @param LinearProgram LP"
                  "# @option RGB min the minimal RGB value"
                  "# @option RGB max the maximal RGB value"
                  "# @return Array<RGB>"
                  "# @example This calculates a vertex coloring with respect to a linear program. For a better visualization,"
                  "# we also set the vertex thickness to 2."
                  "# > $p = cube(3);"
                  "# > $p->LP(LINEAR_OBJECTIVE=>[0,1,2,3]);"
                  "# > $v = vertex_colors($p,$p->LP);"
                  "# > $p->VISUAL(VertexColor=>$v,VertexThickness=>2);",
                  &vertex_colors, "vertex_colors(Polytope LinearProgram { min => undef, max => undef })");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
