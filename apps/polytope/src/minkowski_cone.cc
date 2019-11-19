/* Copyright (c) 1997-2019
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
#include "polymake/Vector.h"
#include "polymake/Graph.h"
#include "polymake/linalg.h"
#include "polymake/graph/Lattice.h"
#include "polymake/graph/Decoration.h"

namespace polymake { namespace polytope {

using Lattice = graph::Lattice<graph::lattice::BasicDecoration, graph::lattice::Sequential>;
        
// computes the cone defined in R^d, where d is the number of edges of the graph G
// defined by the intersection of the positive orthant with all equations given by
// the condition that the edge lengths around any 2-face should add to 0
perl::Object minkowski_cone(const Lattice& H, // we only need the 2-faces
                            const Graph<>& G,      // the graph of the polytope
                            const EdgeMap<Undirected, Vector<Rational>>& edge_directions, // the edge directions of the polytope
                            const Set<int>& far_face)
{
  perl::Object c("Cone<Rational>");  // the minkowski cone
      
  // label the edges consecutively
  // they should come in the same order as given by $polytope->GRAPH->EDGES;
  EdgeMap<Undirected,int> edge_labels(G);
  int i = 0;
  int far_edges = 0;
  for (auto e = entire(edges(G)); !e.at_end(); ++e) {
    if (!far_face.contains(e.from_node()) && !far_face.contains(e.to_node())) {
       edge_labels[*e] = i;
       i++;
    } else {
      far_edges++;
    }
  }
            
  // collect the equations given by the 2-faces
  // order the nodes: smallest, smaller neighbour, then consecutively
  Matrix<Rational> equations(0,G.edges());
  for (const auto node : H.nodes_of_rank(3)) {
    Matrix<Rational> M(edge_directions[0].dim(),G.edges()-far_edges);

    Set<int> TwoFace = H.face(node);  // the 2-face we are currently working on
    if (!(TwoFace*far_face).empty()) continue;

    int v = *(TwoFace.begin());        // the node with smallest index
    TwoFace -= v;                     
    int w = *((TwoFace * G.adjacent_nodes(v)).begin());  // the neighbor of v with smaller index
    TwoFace -= w;
    // add edge  direction to matrix, note that  edge direction is
    // from larger to smaller node index
    M.col(edge_labels(v,w)) = -edge_directions[edge_labels(v,w)];
        
    int u(-1); // need the node index outside loop; initialize to nonsensical value
    // add edges
    while (!TwoFace.empty()) {
      u = *((TwoFace * G.adjacent_nodes(w)).begin());
      Vector<Rational> E = edge_directions[edge_labels(u,w)];
      M.col(edge_labels(u,w)) = (u > w) ? (-1)*E : E;
      TwoFace -= u;
      w = u;
    } 
    // add the edge connecting back to v
    M.col(edge_labels(v,u)) = edge_directions[edge_labels(v,u)];
        
    // the rank of M should always be 2
    equations /= M.minor(basis_rows(M),All);
  }
      
  c.take("EQUATIONS") << equations;

  //inequalities are x_i>=0
  c.take("INEQUALITIES") << unit_matrix<Rational>(G.edges()-far_edges);
      
  return c;
}    

// return the polytope defined by a point in the minkowski summand cone
perl::Object minkowski_cone_point(const Vector<Rational>& cone_point, // the point in the minkowski cone
                                  const Matrix<Rational>& rays, // the rays of the recession cone of the original polytope
                                  const perl::Object g, // graph of the original polytope
                                  const Set<int>& far_face
                                  )
{
  const Graph<> G = g.give("ADJACENCY");
  EdgeMap<Undirected,Vector<Rational>> edge_directions = g.give("EDGE_DIRECTIONS");

  // scale the edge directions
  int i = 0;
  for (auto e = entire(edges(G)); !e.at_end();  ++e) {
    if (!far_face.contains(e.from_node()) && !far_face.contains(e.to_node())) {
      edge_directions[*e] *= cone_point[i];
      i++;
    } else {
      edge_directions[*e] *= 0;
    } 
  }

  NodeMap<Undirected,Vector<Rational> > new_vertices(G);  // the vertices of the Minkowski summand

  // compute a spanning tree rooted at vertex 0
  std::list<int> unprocessed_leaves;
  Bitset marked(G.nodes());  // nodes already included in the tree

  unprocessed_leaves.push_back(0); // we start the tree at the node 0
  marked.insert(0); 
  new_vertices[0] = unit_vector<Rational>(edge_directions[0].dim(),0);  // base vertex of the summand will be the origin

  while ( !unprocessed_leaves.empty() ) {      
    const int current = unprocessed_leaves.front();
    unprocessed_leaves.pop_front();
    Set<int> neighbours = G.adjacent_nodes(current);  
    for (auto v = entire(neighbours); !v.at_end(); ++v) {
      if (!marked.contains(*v)) {
        unprocessed_leaves.push_back(*v);
        marked.insert(*v);
        if ( current < *v )  // edge_directions point from smaller to larger node
          new_vertices[*v] = new_vertices[current] - edge_directions[G.edge(current,*v)]; 
        else
          new_vertices[*v] = new_vertices[current] + edge_directions[G.edge(current,*v)];
      }
    }
  }

  // points for the polytope (not vertices: edges of length zero lead to duplicates in the list)
  perl::Object p("Polytope<Rational>");
  p.take("POINTS") <<  (Matrix<Rational>(new_vertices)/ rays);

  return p;
}
    
    
// return the polytope defined by coefficients to the rays of the minkowski summand cone
perl::Object minkowski_cone_coeff(const Vector<Rational>& coefficients, perl::Object c, const perl::Object g, const Set<int>& far_face, const Matrix<Rational>& tailcone)
{
  Matrix<Rational> rays = c.give("RAYS");      
  if (coefficients.dim() != rays.rows()) 
    throw std::runtime_error( "[minkowski_cone_coeff] -- coefficient vector has wrong dimension");
  return minkowski_cone_point(coefficients * rays, tailcone, g, far_face);
}    

Function4perl(&minkowski_cone, "minkowski_cone($,$,$,$)");

Function4perl(&minkowski_cone_coeff, "minkowski_cone_coeff($,$,$,$,$)");

Function4perl(&minkowski_cone_point, "minkowski_cone_point($,$,$,$)");
        
} }

// Local Variables:
// mode:C++
// c-basic-offset:2
// indent-tabs-mode:nil
// End:
