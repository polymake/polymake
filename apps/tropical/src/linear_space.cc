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
#include "polymake/Array.h"
#include "polymake/Rational.h"
#include "polymake/Set.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Matrix.h"
#include "polymake/ListMatrix.h"
#include "polymake/TropicalNumber.h"
#include "polymake/linalg.h"
#include "polymake/fan/tight_span.h"
#include "polymake/graph/lattice_builder.h"

namespace polymake { namespace tropical {

using namespace graph;
using namespace graph::lattice;
using namespace fan;

template <typename Addition>
BigObject linear_space(BigObject valuated_matroid)
{
  const BigObject polytope = valuated_matroid.give("POLYTOPE");
  const Matrix<Rational> &vertices = polytope.give("VERTICES");
  const auto no_front_set = sequence(1,vertices.cols()-1);
  const auto vertices_no_front = vertices.minor(All,no_front_set);
  const Int n = valuated_matroid.give("N_ELEMENTS");
  Int n_facets = valuated_matroid.give("N_BASES");
  const Vector<TropicalNumber<Addition> > &valuation = valuated_matroid.give("VALUATION_ON_BASES");
  const Vector<Rational> rational_valuation(valuation);
  const Array<Set<Int>>& subdivision = valuated_matroid.give("SUBDIVISION");
  const Array<Array<Set<Int>>>& split_flacets = valuated_matroid.give("SPLIT_FLACETS");
  const Int polytope_dim = polytope.call_method("DIM");
  ListMatrix<Vector<Rational> > new_vertices;

  // Check absence of loops
  const Int n_matroid_loops = valuated_matroid.give("N_LOOPS");
  if (n_matroid_loops > 0) {
    // an empty cycle
    return BigObject("Cycle", mlist<Addition>(),
                     "PROJECTIVE_VERTICES", Matrix<Rational>(0, n+1),
                     "MAXIMAL_POLYTOPES", Array<Set<Int>>(),
                     "PROJECTIVE_AMBIENT_DIM", n-1,
                     "WEIGHTS", Vector<Integer>());
  }

  // excluded faces (those with loops):
  Array<Set<Int>> including_bases(n);
  std::list<Set<Int>> non_including_bases(n);
  const auto total_list = sequence(0,n_facets);
  auto non_bases_it = entire(non_including_bases);
  auto bases_it = entire(including_bases);
  for (auto vcol = entire(cols(vertices_no_front)); !vcol.at_end();
       ++vcol, ++bases_it, ++non_bases_it) {
    *bases_it = support(*vcol);
    *non_bases_it = total_list - *bases_it;
  }

  // Vertices of the tight span (for each max. cell in the subdivision):
  const auto& equations = -rational_valuation | vertices_no_front;
  for (const auto& cell : subdivision) {
    auto nspace = null_space( equations.minor(cell,All));
    auto ncol = entire(nspace.col(0));
    for (auto nrows = entire(rows(nspace)); !nrows.at_end(); ++nrows, ++ncol) {
      if (!ncol->is_zero()) {
        new_vertices /= *nrows / *ncol;
        break;
      }
    }
  }

  // stack the 'loop-free' facets and add a ray for each such facet
  RestrictedIncidenceMatrix<> flacets(subdivision.size(), rowwise(), entire(subdivision));
  Int i = 1;
  for (auto incl_base  = entire(including_bases); !incl_base.at_end(); ++incl_base, ++i) {
    const Int n_loops = attach_selector( cols(vertices_no_front.minor(*incl_base,All)), operations::is_zero()).size();
    if (n_loops > 0 || rank( vertices.minor(*incl_base,All)) != polytope_dim) continue;

    new_vertices /= (Addition::orientation() * unit_vector<Rational>(n+1,i));
    flacets /= ( (*incl_base) + n_facets);
    n_facets++;
  }
  for (auto rk = entire<indexed>(split_flacets); !rk.at_end(); ++rk) {
    for (const auto& flacet : *rk) {
      Vector<Rational> v(n);
      v.slice(flacet).fill(1);
      v = -rk.index() | v;
      auto col_sum_supp = total_list - support(vertices * v);
      if (rank( vertices.minor (col_sum_supp,All)) != polytope_dim) continue;

      v[0] = 0;
      flacets /= col_sum_supp + n_facets;
      new_vertices /= Addition::orientation() * v;
      ++n_facets;
    }
  }

  // Build Hasse diagram
  const IncidenceMatrix<> flacet_inc(std::move(flacets));
  NoBoundaryCut cut( non_including_bases, flacet_inc);
  BasicClosureOperator<> cop(flacet_inc.rows(), T(flacet_inc));
  BasicDecorator<> dec(0, scalar2set(-1));
  Lattice<BasicDecoration> hasse_diagram = lattice_builder::compute_lattice_from_closure<BasicDecoration>(
     cop, cut, dec, 1, lattice_builder::Primal());
  const Int n_max_polys = hasse_diagram.in_adjacent_nodes(hasse_diagram.top_node()).size();

  // Build lineality space
  const Array<Set<Int>>& ccomp = valuated_matroid.give("CONNECTED_COMPONENTS");
  Matrix<Rational> lin_space(ccomp.size()-1, n+1);

  auto lin_no_front = sequence(1, lin_space.cols()-1);
  auto lin_space_no_front = lin_space.minor(All,lin_no_front);
  auto lin_rows = entire(rows(lin_space_no_front));
  auto cc = entire(ccomp);
  if (!cc.at_end()) {
    // We take all connected components but the first.
    while (!(++cc).at_end()) {
      lin_rows->slice(*cc).fill(1);
      ++lin_rows;
    }
  }

  return BigObject("Cycle", mlist<Addition>(),
                   "PROJECTIVE_VERTICES", new_vertices,
                   "HASSE_DIAGRAM", hasse_diagram,
                   "LINEALITY_SPACE", lin_space,
                   "WEIGHTS", ones_vector<Integer>(n_max_polys));
}

UserFunctionTemplate4perl("# @category Tropical linear spaces"
                          "# This computes the tropical linear space (with the coarsest structure) associated to a valuated matroid."
                          "# If you have a trivial valuation, it is highly recommended, you use"
                          "# [[matroid_fan]] instead."
                          "# @param matroid::ValuatedMatroid<Addition,Rational> A valuated matroid, whose value group must be the rationals."
                          "# @return Cycle<Addition>",
                          "linear_space<Addition>(matroid::ValuatedMatroid<Addition>)");
} }
