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
#include "polymake/Integer.h"
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/SparseMatrix.h"
#include "polymake/Vector.h"
#include "polymake/Graph.h"
#include "polymake/graph/compare.h"

namespace polymake { namespace polytope {

void facet_vertex_distance_graph(Graph<>& G, Vector<Int>& C, const SparseMatrix<Int>& D)
{
   Int m = D.cols();
   G.resize(m+D.rows());
   C = ones_vector<Int>(m) | zero_vector<Int>(D.rows());
      
   for (auto r = entire(rows(D)); !r.at_end(); ++r)
      for (auto c = entire(*r); !c.at_end(); ++c) {
         const Int n = G.add_node();
         G.edge(m+r.index(),n);
         G.edge(c.index(),n);
         C |= D[r.index()][c.index()];
      }
}
    
bool lattice_isomorphic_smooth_polytopes(BigObject p1, BigObject p2)
{
   if ( !p1.give("LATTICE") || !p2.give("LATTICE") ) 
      throw std::runtime_error("lattice isomorphism test: polytopes must be lattice polytopes");

   if ( !p1.give("SMOOTH") || !p2.give("SMOOTH") ) 
      throw std::runtime_error("lattice isomorphism test: polytopes must be smooth");
      
   Matrix<Int> D1 = p1.give("FACET_VERTEX_LATTICE_DISTANCES");
   Matrix<Int> D2 = p2.give("FACET_VERTEX_LATTICE_DISTANCES");

   if ( D1.rows() != D2.rows() || D1.cols() != D2.cols() ) 
      return false;

   Graph<> G1, G2;
   Vector<Int> C1, C2;
   facet_vertex_distance_graph(G1,C1,D1);
   facet_vertex_distance_graph(G2,C2,D2);
      
   return graph::isomorphic(G1,C1,G2,C2);
}

Array<Array<Int>> lattice_automorphisms_smooth_polytope(BigObject p)
{
   if ( !p.give("LATTICE") ) 
      throw std::runtime_error("lattice isomorphism test: polytopes must be lattice polytopes");

   if ( !p.give("SMOOTH") ) 
      throw std::runtime_error("lattice isomorphism test: polytopes must be smooth");
      
   Matrix<Int> D = p.give("FACET_VERTEX_LATTICE_DISTANCES");
   const Int n = p.give("N_VERTICES");

   Graph<> G;
   Vector<Int> C;
   facet_vertex_distance_graph(G,C,D);

   Array<Array<Int>> A = graph::automorphisms(G,C);
   for (auto a = entire(A); !a.at_end(); ++a)
      a->resize(n);

   return A;
}


UserFunction4perl("# @category Comparing"
                  "# Tests whether two smooth lattice polytopes are lattice equivalent"
                  "# by comparing lattice distances between vertices and facets. "
                  "# @param Polytope P1 the first lattice polytope"
                  "# @param Polytope P2 the second lattice polytope"
                  "# @return Bool 'true' if the polytopes are lattice equivalent, 'false' otherwise"
                  "# @example"
                  "# > $t = new Vector(2,2);"
                  "# > print lattice_isomorphic_smooth_polytopes(cube(2),translate(cube(2),$t));"
                  "# | true",
                  &lattice_isomorphic_smooth_polytopes, "lattice_isomorphic_smooth_polytopes(Polytope,Polytope)");

UserFunction4perl("# @category Symmetry"
                  "# Returns a generating set for the lattice automorphism group of a smooth polytope //P//"
                  "# by comparing lattice distances between vertices and facets. "
                  "# @param Polytope P the given polytope"
                  "# @return Array<Array<Int>> the generating set for the lattice automorphism group"
                  "# @example"
                  "# > print lattice_automorphisms_smooth_polytope(cube(2));"
                  "# | 2 3 0 1"
                  "# | 1 0 3 2"
                  "# | 0 2 1 3",
                  &lattice_automorphisms_smooth_polytope, "lattice_automorphisms_smooth_polytope(Polytope)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
