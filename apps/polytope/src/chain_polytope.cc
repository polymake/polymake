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

#include "polymake/client.h"
#include "polymake/Matrix.h"
#include "polymake/Rational.h"
#include "polymake/graph/Lattice.h"
#include "polymake/graph/Decoration.h"
#include "polymake/Graph.h"
#include "polymake/PowerSet.h"

namespace polymake { namespace polytope {

namespace {

template <typename Decoration>
Matrix<Int> constraints(const graph::Lattice<Decoration>& HD, const Array<Set<Int>>& max_chains)
{
  const Int d = HD.graph().nodes()-2; // don't count the top and bottom elements
  const Int m = max_chains.size();

  Matrix<Int> Ineq(d+m,d+1);
  // nonnegativity constraints from nontrivial poset elements
  for (Int i = 0; i<d; ++i) {
    Ineq(i,i+1) = 1;
  }
  // nontrivial Ineq from maximal chains
  // the  top and bottom nodes are 0 and d+1, respectively;
  // hence node indices correspond to column indices
  for (Int i=0; i<m; ++i) {
    Ineq(d+i,0) = 1;
    for (auto j=entire(max_chains[i]); !j.at_end(); ++j)
      Ineq(d+i,*j) = -1;
  }

  return Ineq;
}

template <typename Decoration>
Matrix<Int> points(const graph::Lattice<Decoration>& HD, const Array<Set<Int>>& max_anti_chains)
{
  Set<Set<Int>> all_anti_chains;
  for (auto mac=entire(max_anti_chains); !mac.at_end(); ++mac) {
    all_anti_chains += all_subsets(*mac);
  }

  const Int m = all_anti_chains.size();
  const Int d = HD.graph().nodes()-2; // don't count the top and bottom elements
  Matrix<Int> Pts(m,d+1);
  Pts.col(0) = ones_vector<Int>(m); // homogenizing coord

  Int i=0;
  for (auto ac=entire(all_anti_chains); !ac.at_end(); ++ac, ++i) {
    for (auto j=entire(*ac); !j.at_end(); ++j) {
      Pts(i,*j) = 1;
    }
  }
  return Pts;
}

}

template <typename Decoration>
BigObject chain_polytope(BigObject L)
{
  const graph::Lattice<Decoration> HD(L);

  const Int d = HD.graph().nodes()-2; // don't count the top and bottom elements
  const Int top = HD.top_node();
  const Int bottom = HD.bottom_node();
  Set<Int> tb1, tb2;
  tb1 += 0; tb1 += d+1; tb2 += top; tb2 += bottom;
  if (tb1 != tb2)
    throw std::runtime_error("non-standard indices for top and bottom");

  const Array<Set<Int>> max_chains = L.give("MAXIMAL_CHAINS");
  const Matrix<Int> facets = constraints(HD, max_chains);
  const Array<Set<Int>> max_anti_chains = L.give("MAXIMAL_ANTI_CHAINS");
  const Matrix<Int> vertices = points(HD, max_anti_chains);
  const Matrix<Rational> affine_hull(0,d+1);
   
  return BigObject("Polytope<Rational>",
                   "FACETS", facets,
                   "AFFINE_HULL", affine_hull,
                   "VERTICES", vertices,
                   "CONE_DIM", d+1);
}

UserFunctionTemplate4perl("#@category Producing a polytope from graphs"
                          "# Chain polytope of a poset."
                          "# See Stanley, Discr Comput Geom 1 (1986)."
                          "# @param Lattice L"
                          "# @return Polytope<Rational>",
                          "chain_polytope<Decoration>(Lattice<Decoration>)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
