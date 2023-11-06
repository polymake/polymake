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
#include "polymake/graph/graph_iterators.h"
#include "polymake/PowerSet.h"

namespace polymake { namespace polytope {

namespace {

template <typename Decoration>
Matrix<Int> constraints(const graph::Lattice<Decoration>& HD)
{
  const Int d = HD.graph().nodes()-2; // don't count the top and bottom elements
  // facets correspond to covering relations, including those incident with top/bottom
  const Int m = HD.graph().edges(); 
  
  const Int top = HD.top_node();
  const Int bottom = HD.bottom_node();
  Matrix<Int> Ineq(m,d+1);
  Int i = 0;
  // the  top and bottom nodes are 0 and d+1, respectively;
  // hence node indices correspond to column indices
  for (auto j=entire(HD.out_adjacent_nodes(bottom)); !j.at_end(); ++i, ++j) {
    Ineq(i,*j) = 1;
  }
  for (auto j=entire(HD.in_adjacent_nodes(top)); !j.at_end(); ++i, ++j) {
    Ineq(i,0) = 1; Ineq(i,*j) = -1;
  }
  // all other edges
  for (auto e = entire(edges(HD.graph())); !e.at_end(); ++e) {
    if (e.from_node()>0 && e.to_node()<=d) {
      Ineq(i,e.from_node()) = -1; Ineq(i,e.to_node()) = 1;
      ++i;
    }
  }
  
  return Ineq;
}
  
template <typename Decoration>
Matrix<Int> points(const graph::Lattice<Decoration>& HD, const Array<Set<Int>>& max_anti_chains)
{
  const Int d = HD.graph().nodes()-2; // don't count the top and bottom elements
  const Int top = HD.top_node();

  // the setup is chosen such that the vertices of order and chain polytopes match
  Set<Set<Int>> all_anti_chains;
  for (auto mac=entire(max_anti_chains); !mac.at_end(); ++mac) {
    all_anti_chains += all_subsets(*mac);
  }
  
  const Int m = all_anti_chains.size();
  Matrix<Int> Pts(m,d+1);
  Pts.col(0) = ones_vector<Int>(m); // homogenizing coord

  Int i=0;
  for (auto ac=entire(all_anti_chains); !ac.at_end(); ++ac, ++i) {
    auto it=entire(*ac);

    // here the empty anti-chain corresponds to the empty filter
    if (it.at_end()) {
       continue; // nothing to do; matrix initialized as zero
    }

    // nonempty anti-chain
    graph::BFSiterator< Graph<Directed> > j(HD.graph(), *it);
    while (true) {
      while (!j.at_end()) {
        const Int node = *j;
        if (node!=top) Pts(i,node) = 1;
        ++j;
      }
      ++it;
      if (!it.at_end())
        j.process(*it);
      else
        break;
    }
  }
  return Pts;
}

}
      
template <typename Decoration>
BigObject order_polytope(BigObject L)
{
  const graph::Lattice<Decoration> HD(L);

  const Int d = HD.graph().nodes()-2; // don't count the top and bottom elements
  const Int top = HD.top_node();
  const Int bottom = HD.bottom_node();
  Set<Int> tb1, tb2;
  tb1 += 0; tb1 += d+1; tb2 += top; tb2 += bottom;
  if (tb1 != tb2)
    throw std::runtime_error("non-standard indices for top and bottom");

  const Matrix<Int> facets = constraints(HD);
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
                          "# Order polytope of a poset."
                          "# See Stanley, Discr Comput Geom 1 (1986)."
                          "# @param Lattice L"
                          "# @return Polytope<Rational>",
                          "order_polytope<Decoration>(Lattice<Decoration>)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
