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

#pragma once

#include "polymake/client.h"
#include "polymake/Graph.h"
#include "polymake/Set.h"
#include "polymake/Vector.h"
#include "polymake/Matrix.h"
#include "polymake/graph/graph_iterators.h"
#include "polymake/graph/DoublyConnectedEdgeList.h"
#include "polymake/topaz/DomeVolumeVisitor.h"
#include <cmath>

namespace polymake { namespace topaz {

class CoveringTriangulationVisitor : public graph::NodeVisitor<> {
public:
   // A decorated edge consists of the half edge index and a matrix for the two decorating horocycles.
   using DecoratedEdge = std::pair<Int, Matrix<Rational>>;
   // Each DecoratedEdge that appears in the BFS will get an unique index via this map.
   using EdgeMap = Map<Int, DecoratedEdge>;

   using DoublyConnectedEdgeList = graph::dcel::DoublyConnectedEdgeList;
   using HalfEdge = graph::dcel::HalfEdge;

private:
   // The graph we want to iterate through, build during iterating.
   // Part of the dual spanning tree of the triangulation of the universal covering H^2.
   Graph<Directed>& dual_tree;

   // The triangulation of the surface.
   DoublyConnectedEdgeList& dcel;

   const Vector<Rational> angleVec;

   // A map from the node-indices from the dual_tree to a pair (half edge , horocycles [[p_tail,q_tail],[p_head,q_head]]).
   EdgeMap edge_map;

   // The vertices of the triangulation.
   std::vector<Vector<Rational>> points;

   // Mapping horocycles to their positions in points
   Map<Vector<Rational>, Int> vertex_map;

   // The number of nodes in the dual graph.
   const Int num_nodes_depth;

   // The triangles of the triangulation, triplets of corresponding indices.
   Array<Set<Int>> triangles;

   Int curr_num_nodes;

   // count of the nodes that were visited already.
   Int num_visited;

public:

   // This is needed for the BFS-iterator to not just stop in depth one.
   static constexpr bool visit_all_edges = true;

   /*
     The CTV is initialized with the given graph G (usually only one node for the first half edge 0 / first triangle (0,0->next,0->next->next) ),
     the triangulation dcel (containing the lambda lengths of the edges),
     the position of the first half edge via two horocycles [[p_1,q_1] , [p_2,q_2]]
     and the depth we want to visit the dual tree.
   */
   CoveringTriangulationVisitor(Graph<Directed>& G, DoublyConnectedEdgeList& dcel_, const Matrix<Rational>& first_halfedge_horo, Int dual_tree_depth)
      : dual_tree(G)
      , dcel(dcel_)
      , angleVec(dcel.angleVector())
      , num_nodes_depth(3*((1L << dual_tree_depth)-1)+1)
      , triangles(num_nodes_depth)
      , curr_num_nodes(0)
      , num_visited(0)
   {
      layFirstEdge(first_halfedge_horo);
   }

   Int numVisited() const { return num_visited; }

   bool operator()(Int n)
   {
      return operator()(n, n);
   }

   bool operator()(Int n_from, Int n_to)
   {
      if (visited.contains(n_to)) return false;
      /*
        The node n_to was not visited yet, therefore the corresponding triangle (its known halfedge) did not contribute to the GKZ vector.
        We extract the known half edge as well as the two known horocycles from the edge_map.
        Then we compute the position of the third horocycle, and put two new adjacent triangles (half edges) in the queue.
      */
      const DecoratedEdge& edge_pair = edge_map[n_to];
      Vector<Rational> horo_u = edge_pair.second[0];
      Vector<Rational> horo_v = edge_pair.second[1];
      const HalfEdge* uv = dcel.getHalfEdge(edge_pair.first);
      const HalfEdge* vw = uv->getNext();
      const HalfEdge* wu = vw->getNext();
      const Rational& lambda_uv = uv->getLength();
      const Rational& lambda_vw = vw->getLength();
      const Rational& lambda_wu = wu->getLength();
      Int u_id = dcel.getVertexId(wu->getHead());
      Int v_id = dcel.getVertexId(uv->getHead());
      Int w_id = dcel.getVertexId(vw->getHead());

      Vector<Rational> horo_w = thirdHorocycle(horo_u, horo_v, lambda_uv, lambda_vw, lambda_wu);

      Rational u_scaling = 1 / angleVec[u_id];
      Rational v_scaling = 1 / angleVec[v_id];
      Rational w_scaling = 1 / angleVec[w_id];

      addVertex(horo_w, w_scaling);
      triangles[n_to] = Set<Int>{ vertex_map[horo_u], vertex_map[horo_v], vertex_map[horo_w] };

      if (dual_tree.nodes() < num_nodes_depth) {
         const HalfEdge* wv = vw->getTwin();
         const HalfEdge* uw = wu->getTwin();

         // Add the two new half edges to the queue, update the edge_map and the dual_tree.
         // Note that we change the sign of the second horocycle (head of wv and uw) since we want to guarantee a positive determinant.

         const Matrix<Rational> M_wv = vector2row(horo_w) / -horo_v;
         const Matrix<Rational> M_uw = vector2row(horo_u) / -horo_w;

         const Int wv_node_id = dual_tree.add_node();
         dual_tree.add_edge(n_to, wv_node_id);
         edge_map[wv_node_id] = DecoratedEdge(dcel.getHalfEdgeId(wv), M_wv);
         const Int uw_node_id = dual_tree.add_node();
         dual_tree.add_edge(n_to, uw_node_id);
         edge_map[uw_node_id] = DecoratedEdge(dcel.getHalfEdgeId(uw), M_uw);

         curr_num_nodes += 2;
      }

      visited += n_to;
      ++num_visited;
      return true;
   }

   /*
     Apply the map {horocycles in H^2} -> {vertices at Klein boundary}, horovector = [p,q] |-> v = 1/(p^2+q^2) [ q^2-p^2 , 2pq ].
     The vertex v becomes a new row of the matrix points.
     The vertex map at [p,q] is set to the the index of the row v stands in (with index shift due to counting from 0)
   */
   void addVertex(const Vector<Rational>& horo_vector, const Rational& scaling)
   {
      const Rational& p = horo_vector[0];
      const Rational& q = horo_vector[1];
      const Rational a =  p*p + q*q;
      const Vector<Rational> v{ (q*q - p*p)/a, 2*p*q/a, scaling/(a*a) };
      vertex_map[ horo_vector ] = points.size();
      vertex_map[ -horo_vector ] = points.size();
      points.push_back(v);
   }

   // Lay out the first half edge (index=0) in H^2, according to the input of the two horocycles (two rows of the matrix first_halfedge_horo).
   void layFirstEdge(const Matrix<Rational>& horo_matrix)
   {
      addVertex(horo_matrix[0], 1/angleVec[0]);
      addVertex(horo_matrix[1], 1/angleVec[1]);

      edge_map[0] = DecoratedEdge(0, horo_matrix);

      const Matrix<Rational> M_twin_edge = vector2row(horo_matrix[1]) / -horo_matrix[0];

      const Int new_node_id = dual_tree.add_node();
      dual_tree.add_edge(0, new_node_id);
      edge_map[new_node_id] = DecoratedEdge(1, M_twin_edge);

      curr_num_nodes += 2;
   }

   Matrix<Rational> getPoints() const
   {
      return Matrix<Rational>(points);
   }

   const Array<Set<Int>>& getTriangles() const
   {
      return triangles;
   }

}; // end class covering triangulation volume visitor

class CoveringBuilder {
   Graph<Directed> dual_tree; // part of the dual tree of the triangulation
   Int cur_depth;
   graph::BFSiterator< Graph<Directed>, graph::VisitorTag<CoveringTriangulationVisitor> > bfs_it;

public:

   // construct from a dcel and the horo matrix of the first edge
   CoveringBuilder(graph::DoublyConnectedEdgeList& dcel, const Matrix<Rational>& first_halfedge_horo, Int depth_in)
      : dual_tree(1) //start with a one-node graph
      , cur_depth(depth_in)
      , bfs_it(dual_tree, CoveringTriangulationVisitor(dual_tree, dcel, first_halfedge_horo, depth_in), nodes(dual_tree).front())
   {}

   Int getDepth() const { return cur_depth; }

   // get the covering triangulation up to a given depth
   BigObject computeCoveringTriangulation()
   {
      Int num_nodes = 3*(pow(2, cur_depth)-1)+1; //number of nodes of a binary tree with ternary root of given depth

      while (bfs_it.node_visitor().numVisited() < num_nodes) {
         ++bfs_it;
      }

      Matrix<Rational> points = ones_vector<Rational>() | bfs_it.node_visitor().getPoints();
      Array<Set<Int>> triangles = bfs_it.node_visitor().getTriangles();

      return BigObject("fan::PolyhedralComplex<Rational>",
                       "POINTS", points,
                       "INPUT_POLYTOPES", triangles);
   }
};

} }


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
