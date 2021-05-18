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
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/Graph.h"

namespace polymake { namespace polytope {

template <typename Scalar>
Graph<Directed> dgraph(BigObject p, BigObject lp, OptionSet options)
{
   Graph<Directed> DG=p.give("GRAPH.ADJACENCY");
   const Matrix<Scalar> V=p.give("VERTICES");

   const bool inverse=options["inverse"], generic=options["generic"];
   bool upper_bound=true, lower_bound=true, check_for_rays=false;
   Vector<Scalar> obj;
   Set<Int> rays;

   if (lp.lookup("LINEAR_OBJECTIVE") >> obj) {
      obj=V*obj;
      p.give("FAR_FACE") >> rays;
      check_for_rays=!rays.empty();
   } else {
      lp.give("ABSTRACT_OBJECTIVE") >> obj;
   }
   if (inverse) obj.negate();

   auto obj_value=obj.begin();
   for (auto n=entire(nodes(DG));  !n.at_end();  ++n, ++obj_value) {
      if (check_for_rays && rays.contains(*n)) {
         const Int ray_orientation = sign(*obj_value);
         if (ray_orientation > 0) upper_bound = false;
         if (ray_orientation < 0) lower_bound = false;
         if (ray_orientation >= 0) n.out_edges().clear();
         if (ray_orientation <= 0) n.in_edges().clear();
      } else {
         // affine vertex
         for (auto e=n.out_edges().find_nearest(*n, operations::gt()); !e.at_end(); ) {
            if (check_for_rays && rays.contains(e.to_node())) {
               ++e;
               continue;
            }
            Int idiff = sign(*obj_value - obj[e.to_node()]);
            if (idiff == 0 && generic)
               idiff = lex_compare(V.row(*n), V.row(e.to_node())) == (inverse ? pm::cmp_lt : pm::cmp_gt) ? 1 : -1;

            if (idiff <= 0)
               n.in_edges().erase(e.to_node());
            if (idiff >= 0)
               n.out_edges().erase(e++);
            else
               ++e;
         }
      }
   }

   if (!inverse && !generic) {
      Set<Int> minface, maxface;
      for (auto n=entire(nodes(DG));  !n.at_end();  ++n) {
         if (!n.in_degree()) minface.push_back(n.index());
         if (!n.out_degree()) maxface.push_back(n.index());
      }
      lp.take("MINIMAL_FACE") << minface;
      lp.take("MAXIMAL_FACE")  << maxface;
      if (lower_bound)
         lp.take("MINIMAL_VALUE") << obj[minface.front()];
      else
         lp.take("MINIMAL_VALUE") << -std::numeric_limits<Scalar>::infinity();
      if (upper_bound)
         lp.take("MAXIMAL_VALUE") << obj[maxface.front()];
      else
         lp.take("MAXIMAL_VALUE") << std::numeric_limits<Scalar>::infinity();
   }
   return DG;
}

template <typename Scalar>
Vector<Scalar> objective_values_for_embedding(BigObject p, BigObject lp)
{
   const Matrix<Scalar> V = p.give("VERTICES");
   const Vector<Scalar> Obj = lp.give("LINEAR_OBJECTIVE");
   Vector<Scalar> val = V*Obj;
   const Set<Int> rays = p.give("FAR_FACE");
   if (!rays.empty()) {
      const Scalar max_obj=accumulate(val.slice(~rays), operations::max()),
         min_obj=accumulate(val.slice(~rays), operations::min());
      for (auto r=entire(rays); !r.at_end(); ++r)
         if (val[*r]>0)
            val[*r]=2*max_obj-min_obj;
         else
            val[*r]=2*min_obj-max_obj;
   }
   return val;
}

/* @category Optimization
 Direct the graph of a polytope //P// according to a linear or abstract objective function.
 The maximal and minimal values, which are attained by the objective function, as well
 as the minimal and the maximal face are written into separate sections.
 The option //inverse// directs the graph with decreasing instead of increasing edges.
 If the option //generic// is set, ties will be broken by lexicographical ordering.
 @param Polytope P
 @param LinearProgram LP
 @option Bool inverse inverts the direction
 @option Bool generic breaks ties
 @return Graph<Directed>
*/

FunctionTemplate4perl("dgraph<Scalar>(Polytope<Scalar>, LinearProgram<Scalar> { inverse => undef, generic => undef })");

FunctionTemplate4perl("objective_values_for_embedding<Scalar>(Polytope<Scalar> LinearProgram<Scalar>)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
