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

   Computes geometric properties of a MatroidRingCycle.
   */

#include "polymake/client.h"
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/linalg.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/tropical/misc_tools.h"
#include "polymake/tropical/thomog.h"
#include "polymake/tropical/specialcycles.h"

namespace polymake { namespace tropical {

// Finds a vertex in a list of row vectors.
int find_index(const Vector<Rational>& v, const Matrix<Rational>& m)
{
  int i = 0;
  for (auto r = entire(rows(m)); !r.at_end(); ++r, ++i) {
    if (*r == v) return i;
  }
  throw std::runtime_error("Vertex not found");
}

/*
 * @brief Takes a list of tropical cycles which are all defined on (a subset of)
 * the same polyhedral structure and computes their cycle sum
 * @return Cycle<Addition>
 */
template <typename Addition>
perl::Object add_refined_cycles(Array<perl::Object> cycles)
{
  Array<Matrix<Rational>> vertices(cycles.size());
  Matrix<Rational> vertex_union;
  Array<IncidenceMatrix<>> cones(cycles.size());
  Array<Vector<Integer> > weights(cycles.size());
  for (int i =0; i < cycles.size(); ++i) {
    cycles[i].give("VERTICES") >> vertices[i];
    vertex_union /= vertices[i];
    cycles[i].give("MAXIMAL_POLYTOPES") >> cones[i];
    cycles[i].give("WEIGHTS") >> weights[i];
  }
  Set<Vector<Rational> > vset (rows(Matrix<Rational>(vertex_union)));
  Matrix<Rational> vertices_total(vset);

  // Renumber vertices and map cones
  Vector<Set<int> > new_cones;
  Map<Set<int>, int> new_cone_indices;
  int next_index = 0;
  Vector<Integer> new_weights;
  for (int i = 0; i < cycles.size(); ++i) {
    Map<int,int> vmap;
    for (int r =0; r < vertices[i].rows(); ++r) {
      vmap[r] = find_index(vertices[i].row(r), vertices_total);
    }
    int cindex = 0;
    for (auto c = entire(rows(cones[i])); !c.at_end(); ++c, ++cindex) {
      Set<int> mapped_cone{ vmap.map(*c) };
      Integer cw = (weights[i])[cindex];
      if (!new_cone_indices.contains(mapped_cone)) {
        new_cone_indices[mapped_cone] = next_index;
        new_cones |= mapped_cone;
        new_weights |= cw;
        ++next_index;
      } else {
        new_weights[ new_cone_indices[mapped_cone] ] += cw;
      }
    }
  }

  // Remove zero-weight cones
  Set<int> supp = support(new_weights);
  Set<int> used_vertices = accumulate( new_cones.slice(supp), operations::add());
  IncidenceMatrix<> new_cones_matrix(new_cones);

  perl::Object result("Cycle", mlist<Addition>());
  result.take("VERTICES") << vertices_total.minor(used_vertices,All);
  result.take("MAXIMAL_POLYTOPES") << new_cones_matrix.minor(supp, used_vertices);
  result.take("WEIGHTS") << new_weights.slice(supp);
  return result;
}

FunctionTemplate4perl("add_refined_cycles<Addition>(Cycle<Addition>+)");

} }
