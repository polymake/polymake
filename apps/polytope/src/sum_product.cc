/* Copyright (c) 1997-2022
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
#include "polymake/list"
#include "polymake/Graph.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Array.h"
#include "polymake/Vector.h"
#include "polymake/Matrix.h"
#include "polymake/ListMatrix.h"
#include "polymake/linalg.h"

namespace polymake { namespace polytope {

// The graphs that we read already come with a (translation) vector specified for each edge.
// We assume that the graph is acyclic with a unique sink.  Arbitrarily many sources allowed.
typedef Graph<Directed> graph;

// Gathers next generation of graph nodes which can be defined now, since all predecessors known.
void add_next_generation(std::list<Int>& next_nodes, const Int v, const graph& G, const NodeMap<Directed,BigObject>& pa)
{
   for (auto e = entire(G.out_edges(v)); !e.at_end(); ++e) {
      const Int x = e.to_node();
      auto f=entire(G.in_edges(x));
      for( ; !f.at_end() && pa[f.from_node()].valid(); ++f);
      if (f.at_end())
         next_nodes.push_back(x);
   }
}

// Returns translation matrix (to be applied to row vectors from the right) for given vector.
template<typename Scalar>
Matrix<Scalar> translation_by(const Vector<Scalar>& vec)
{
   const Int d = vec.dim();
   return unit_vector<Scalar>(d+1,0) | (vec / unit_matrix<Scalar>(d));
}

// Sum-product algorithm for polytopes.
// Operates on one Poly object from which the SUM_PRODUCT_GRAPH is read.
// Writes the VERTICES and VERTEX_NORMALS of the polytope corresponding to
// the unique sink in the graph.
template<typename Scalar>
void sum_product(BigObject p)
{
   // Read the graph.
   const graph G=p.give("SUM_PRODUCT_GRAPH.ADJACENCY");
   const EdgeMap<Directed, Vector<Scalar>> Trans=p.give("SUM_PRODUCT_GRAPH.TRANSLATIONS");
   const Int n = G.nodes();
   if (n == 0)
      throw std::runtime_error("SUM_PRODUCT_GRAPH must be non-empty");

   // The dimension of the ambient space.
   const Int d = p.call_method("AMBIENT_DIM");

   // This is the description of the origin as a 0-dimensional polytope (living in d-space).
   // Used to initialize the computation at the sources of the graph.
   const Matrix<Scalar> single_point_vertices(vector2row(unit_vector<Scalar>(d+1,0)));
   const IncidenceMatrix<> single_point_vif;

   // This is where all the intermediate polytopes are stored.
   // The nodes in the graph are consecutively numbered, starting at 0.
   // The corresponding polytope can be accessed by indexing with the node number.
   // In the beginning the polytopes are undefined.
   NodeMap<Directed, BigObject> pa(G);
   std::list<Int> next_nodes;
   // will need this now and again
   BigObjectType Polytope("Polytope", mlist<Scalar>());

   // Initialize by assigning a single point (origin) to each source in the graph.
   for (Int v = 0; v < n; ++v) {
      if (G.in_degree(v) == 0) {
         pa[v]=BigObject(Polytope);
         pa[v].take("VERTICES") << single_point_vertices;
         pa[v].take("VERTICES_IN_FACETS") << single_point_vif;
         add_next_generation(next_nodes,v,G,pa);
      }
   }

   // At each node of the graph recursively define a polytope as the convex hull of
   // the translated predecessors.
   // We also try to find the sink on the way.
   Int sink = -1;  // no valid node number; indicates sink not found yet
   while (!next_nodes.empty()) {
      // Get some node w for which we know all the predecessors.
      const Int w = next_nodes.front(); next_nodes.pop_front();
      if (pa[w].valid())
         throw std::runtime_error("unvisited node already initialized");
      pa[w]=BigObject(Polytope);

      // The polytope will be specified as the convex hull of points, which will be collected
      // from other polytopes.
      // The special data type ListMatrix is efficient in terms of concatenating rows
      // (which correspond to points); not efficient in terms of matrix operations
      // (although all operations are defined), but this is ok, since we do not compute
      // anything in this step.
      ListMatrix< Vector<Scalar> > points(0,d+1);
      for (auto e = entire(G.in_edges(w)); !e.at_end(); ++e) {
         // Node v is the current predecessor to process.
         const Int v = e.from_node();
         // Translation vector in the edge from v to w.
         const Vector<Scalar> vec = Trans[*e];
         // We read the vertices of the predecessor polytope.  polymake's rule basis by
         // default uses cdd' implementation to check for redundant (= non-vertex) points
         // among the input by solving linear programs.
         // No convex hull computation necessary.
         const Matrix<Scalar> these_vertices = pa[v].give("VERTICES");
         // Concatenates the translated matrix (where the rows correspond to the vertices
         // of the predecessor) to what we already have.
         points /= these_vertices*translation_by(vec);
      }
      // Define the polytope as the convex hull of all those points.
      pa[w].take("POINTS") << points;
      if (G.out_degree(w) == 0)
         sink=w; // We did find a sink.
      else
         add_next_generation(next_nodes,w,G,pa);
   }

   // This will be just any sink; indeterministic if graph has several sinks.
   if (sink<0)
      throw std::runtime_error("no sink found in digraph");

   const Matrix<Scalar> sink_vertices = pa[sink].give("VERTICES");
   const Matrix<Scalar> sink_normals = pa[sink].give("VERTEX_NORMALS");

   // The sink defines the polytope we are after.
   p.take("VERTICES") << sink_vertices;
   p.take("VERTEX_NORMALS") << sink_normals;
   Matrix<Scalar> empty_lin_space(0,sink_vertices.cols());
   p.take("LINEALITY_SPACE") << empty_lin_space;
}

FunctionTemplate4perl("sum_product<Scalar>(Polytope<Scalar>)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
