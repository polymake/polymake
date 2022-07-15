/*
   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA  02110-1301, USA.

   ---
   Copyright (C) 2011 - 2015, Simon Hampe <simon.hampe@googlemail.com>

   ---
   Copyright (c) 2016-2022
   Ewgenij Gawrilow, Michael Joswig, and the polymake team
   Technische Universität Berlin, Germany
   https://polymake.org

   Contains functions to compute the star of a cycle at a point.
   */

#include "polymake/client.h"
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/linalg.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/polytope/convex_hull.h"
#include "polymake/tropical/convex_hull_tools.h"
#include "polymake/tropical/thomog.h"
#include "polymake/tropical/star.h"

namespace polymake { namespace tropical {

using StarResult = std::pair< Matrix<Rational>, std::vector<Set<Int>>>;

template <typename Addition>
BigObject normalized_star_data(BigObject local_cycle, const Vector<Rational>& point)
{
  const Matrix<Rational>& local_vertices = local_cycle.give("VERTICES");
  const IncidenceMatrix<>& local_cones = local_cycle.give("MAXIMAL_POLYTOPES");
  Matrix<Rational> lineality = local_cycle.give("LINEALITY_SPACE");
  Vector<Integer> weights = local_cycle.give("WEIGHTS");
  StarResult r = computeStar(point, local_vertices, local_cones);

  // Find all zero rays and make the first coordinate a one
  Set<Int> apices = indices( attach_selector( rows(r.first), operations::is_zero()));
  r.first.col(0).slice(apices).fill(1);
  Matrix<Rational> vertices(0, r.first.cols());
  Vector<Int> vertex_map_vector = insert_rays(vertices, r.first, true);
  Map<Int, Int> vertex_map;
  for (auto vm = entire<indexed>(vertex_map_vector); !vm.at_end(); ++vm) {
    vertex_map[ vm.index() ] = *vm;
  }

  // Map cones to new ray indices
  RestrictedIncidenceMatrix<> polytopes;
  for (auto cone : r.second) {
    Set<Int> new_cone ( attach_operation( cone, pm::operations::associative_access<Map<Int, Int>, Int>(&vertex_map)));
    polytopes /= new_cone;
  }
  IncidenceMatrix<> final_polytopes(std::move(polytopes));

  // Now find redundant rays
  Set<Int> used_rays;

  Matrix<Rational> lineality_dehom = tdehomog(lineality);
  for (auto p = entire(rows(final_polytopes)); !p.at_end(); ++p) {
    const Set<Int> can_rays = polytope::get_non_redundant_points(vertices.minor(*p, All), lineality_dehom, true).first;
    used_rays += select(*p, can_rays);
  }

  return BigObject("Cycle", mlist<Addition>(),
                   "VERTICES", thomog(vertices.minor(used_rays, All)),
                   "MAXIMAL_POLYTOPES", final_polytopes.minor(All, used_rays),
                   "LINEALITY_SPACE", lineality,
                   "WEIGHTS", weights);
}

template <typename Addition>
BigObject star_at_vertex(BigObject cycle, Int vertex_index)
{
  BigObject local_cycle = call_function("local_vertex", cycle, vertex_index);
  const Matrix<Rational>& vertices = cycle.give("VERTICES");
  return normalized_star_data<Addition>(local_cycle, vertices.row(vertex_index));
}

template <typename Addition>
BigObject star_at_point(BigObject cycle, const Vector<Rational>& point)
{
  BigObject local_cycle = call_function("local_point",cycle, point);
  return normalized_star_data<Addition>(local_cycle, point);
}


UserFunctionTemplate4perl("# @category Local computations"
                          "# Computes the Star of a tropical cycle at one of its vertices."
                          "# @param Cycle<Addition> C a tropical cycle"
                          "# @param Int i The index of a vertex in [[VERTICES]], which should not be a ray"
                          "# @return Cycle<Addition> The Star of C at the vertex",
                          "star_at_vertex<Addition>(Cycle<Addition>,$)");

UserFunctionTemplate4perl("# @category Local computations"
                          "# Computes the Star of a tropical cycle at an arbitrary point in its support"
                          "# @param Cycle<Addition> C a tropical cycle "
                          "# @param Vector<Rational> v A point, given in tropical projective coordinates with"
                          "# leading coordinate and which should lie in the support of C"
                          "# @return Cycle<Addition> The Star of C at v (Note that the subdivision may be finer than"
                          "# a potential coarsest structure",
                          "star_at_point<Addition>(Cycle<Addition>,Vector<Rational>)");
} }
