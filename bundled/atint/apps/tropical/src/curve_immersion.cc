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
	Copyright (c) 2016-2023
	Ewgenij Gawrilow, Michael Joswig, and the polymake team
	Technische Universität Berlin, Germany
	https://polymake.org

	Contains functions to compute the immersion of a rational curve, given a certain degree. 
	*/

#include "polymake/client.h"
#include "polymake/Matrix.h"
#include "polymake/Rational.h"
#include "polymake/Vector.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/common/lattice_tools.h"
#include "polymake/tropical/lattice.h"
#include "polymake/tropical/thomog.h"
#include "polymake/tropical/make_complex.h"
#include "polymake/tropical/misc_tools.h"

namespace polymake { namespace tropical {

// Documentation see perl wrapper
template <typename Addition>
BigObject rational_curve_immersion(const Matrix<Rational>& delta_, BigObject type)
{
  const Matrix<Rational> delta = tdehomog(delta_, 0, false);
  // Extract values
  IncidenceMatrix<> nodes_by_sets = type.give("NODES_BY_SETS");
  IncidenceMatrix<> nodes_by_leaves = type.give("NODES_BY_LEAVES");
  IncidenceMatrix<> sets = type.give("SETS");
  Vector<Int> coeffs = type.give("COEFFS");
  const Int n_leaves = type.give("N_LEAVES");
  if (n_leaves != delta.rows())
    throw std::runtime_error("Cannot create curve embedding. Degree does not match number of leaves");

  Int ambient_dim = delta.cols();

  // Result variables
  // The first k rows contain the nodes in order of their appearance in
  // NODES_BY_*
  Matrix<Rational> rays(nodes_by_leaves.rows(), ambient_dim+1); 
  Vector<Set<Int>> cones;
  Vector<Integer> weights;

  // This vector tells us whether the position of a vertex has already been computed
  const Int n_vertices = nodes_by_leaves.rows();
  std::vector<bool> computed(n_vertices);

  // We arbitrarily place the first node at (0,...,0)
  rays.row(0) = unit_vector<Rational>(rays.cols(),0);
  computed[0] = true;

  // Put all remaining nodes in a queue
  // Then iterate: When a node doesn't have a neighbour whose position has been computed yet,
  // move it to the back
  std::list<Int> queue;
  for (Int v = 1; v < nodes_by_sets.rows(); v++) {
    queue.push_back(v);
  }

  while (!queue.empty()) {
    Int nextv = queue.front();
    queue.pop_front();
    Int neighbour = -1;
    for (Int nb = 0; nb < n_vertices; ++nb) {
      if (nb != nextv && computed[nb] && (nodes_by_sets.row(nb) * nodes_by_sets.row(nextv)).size() > 0) {
        neighbour = nb; break;
      }
    } //END search for neighbours
    if (neighbour == -1) {
      queue.push_back(nextv);
      continue;
    }

    // Compute orientation of edge: Take any other edge at nextv. It the intersection with
    // the partition of the connecting edge is empty or the connecting set, then 
    // the edge points away from nextv, otherwise towards it
    Int edge_index = *((nodes_by_sets.row(neighbour) * nodes_by_sets.row(nextv)).begin());
    Set<Int> edge_set = sets.row(edge_index);
    Set<Int> compare_set;
    if (nodes_by_leaves.row(nextv).size() > 0) {
      compare_set += *(nodes_by_leaves.row(nextv).begin());
    } else {
      Int otherset = *((nodes_by_sets.row(nextv) - edge_index).begin());
      compare_set = sets.row(otherset);
    }
    Int sign = +1;
    Set<Int> inter = edge_set * compare_set;
    if (inter.size() == 0 || inter.size() == edge_set.size()) {
      sign = -1;
    }

    // Now compute the vertex
    Vector<Rational> sum_of_leaves(rays.cols()-1);
    for (auto d = entire(edge_set); !d.at_end(); ++d) {
      sum_of_leaves += delta.row(*d-1);
    }
    sum_of_leaves = Rational(0) | sum_of_leaves;
    // Compute the weight of the bounded edge as the multiplicity of the 
    // directional ray wrt to the primitive one
    Vector<Rational> direction = sign * sum_of_leaves;
    Vector<Integer> primitive_direction = common::primitive(direction);
    Integer mult; 
    for (Int x = 0; x < direction.dim(); ++x) {
      if (direction[x] != 0) {
        mult = direction[x] / primitive_direction[x];
        break;
      }
    }
    rays.row(nextv) = rays.row(neighbour) + coeffs[edge_index] * primitive_direction;
    computed[nextv] = true;
    weights |= mult;

    // Create the cone
    Set<Int> cone_set;
    cone_set += nextv; cone_set += neighbour;
    cones |= cone_set;
  } // END compute vertices

  // Finally we attach the leaves - but there may be doubles!
  Matrix<Rational> leaves = zero_vector<Rational>() | delta;
  Map<Int, Int> leaf_ray_index;
  for (Int l = 0; l < leaves.rows(); ++l) {
    Int index = -1;
    Vector<Integer> primitive_leaf = common::primitive(leaves.row(l));
    Integer mult;
    for (Int x = 0; x < primitive_leaf.dim(); x++) {
      if (primitive_leaf[x] != 0) {
        mult = leaves(l,x) / primitive_leaf[x];
        break;
      }
    }
    weights |= mult;
    for (Int r = nodes_by_leaves.rows(); r < rays.rows(); ++r) {
      if (rays.row(r) == leaves.row(l)) {
        index = r;
        break;
      }
    }
    if (index == -1) {
      rays /= leaves.row(l);
      leaf_ray_index[l] = rays.rows()-1;
    } else {
      leaf_ray_index[l] = index;
    }
  }

  for (Int v = 0; v < nodes_by_leaves.rows(); ++v) {
    Set<Int> leaves_here = nodes_by_leaves.row(v);
    if (leaves_here.size() > 0) {
      for (auto l = entire(leaves_here); !l.at_end(); ++l) {
        const Set<Int> cone_set{ v, leaf_ray_index[*l-1] };
        cones |= cone_set;
      }
    }
  }

  // We need to make this a polyhedral complex.
  return make_complex<Addition>(thomog(Matrix<Rational>(rays)), cones, weights);
}

// ------------------------- PERL WRAPPERS ---------------------------------------------------

UserFunctionTemplate4perl("# @category Abstract rational curves"
                          "# This function creates an embedding of a rational tropical curve using"
                          "# a given abstract curve and degree"
                          "# @param Matrix<Rational> delta The degree of the curve in tropical projectve "
                          "# coordinates without leading coordinate. The number of rows"
                          "# should correspond to the number of leaves of type and the number of columns"
                          "# is the dimension of the space in which the curve should be realized"
                          "# @param RationalCurve type An abstract rational curve"
                          "# @tparam Addition Min or Max"
                          "# @return Cycle<Addition> The corresponding immersed complex."
                          "# The position of the curve is determined by the first node, "
                          "# which is always placed at the origin",
                          "rational_curve_immersion<Addition>($, RationalCurve)");
} }
