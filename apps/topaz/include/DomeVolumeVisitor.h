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

#pragma once

#include "polymake/client.h"
#include "polymake/Graph.h"
#include "polymake/Set.h"
#include "polymake/Vector.h"
#include "polymake/Matrix.h"
#include "polymake/graph/graph_iterators.h"
#include "polymake/graph/DoublyConnectedEdgeList.h"

namespace polymake { namespace topaz {

// Forward declaration of the function that computes the third horocycle, given the three lambda-lengths of a triangle and two of its horocycles (given via [p,q]-identification).
Vector<Rational> thirdHorocycle(const Vector<Rational>& horo_u, const Vector<Rational>& horo_v, const Rational& lambda_uv, const Rational& lambda_vw, const Rational& lambda_wu);

class DomeVolumeVisitor : public graph::NodeVisitor<> {
public:
   // A decorated edge consists of the half edge index and a matrix for the two decorating horocycles.
   using DecoratedEdge = std::pair<Int, Matrix<Rational>>;

   // Each DecoratedEdge that appears in the BFS will get an unique index via this map.
   using EdgeMap = Map<Int, DecoratedEdge>;

   using DoublyConnectedEdgeList = graph::dcel::DoublyConnectedEdgeList;
   using HalfEdge = graph::dcel::HalfEdge;

private:
   // The graph we want to iterate through, built during iteration.
   // Part of the dual spanning tree of the triangulation of the universal covering H^2.
   Graph<Directed>& dome_graph;

   // The triangulation of the surface.
   DoublyConnectedEdgeList& dcel;

   // A map from the node-indices from the dome_graph to a pair of a half edge and its two horocycles.
   EdgeMap edge_map;

   // The GKZ vector. Updated during iteration.
   Vector<Rational> gkz_vector;

   // A global lower bound for the prism volumes.
   // If the volume of a prism gets smaller we stop the computation of the GKZ vector in that direction.
   Rational lower_bound_volume;

   /*
     Horocycle scaling:
     By definition the GKZ vectors should be computed w.r.t. a fixed weight (1,...,1).
     The surfaces we are given do not have these weights, but instead the weights that come with the
     chosen lambda lengths (the angle sums = angleVec).
     We calculate the prism volumes w.r.t. these weights and rescale them afterwards.
   */
   Vector<Rational> angleVec;

   // count of the nodes that were visited already.
   Int num_visited;

public:

   // This is needed for the BFS-iterator to not just stop in depth one.
   static constexpr bool visit_all_edges = true;

   /*
     The DVV is initialized with the given graph G (usually only one node for the first half edge),
     the triangulation dcel (containing the lambda lengths of the edges),
     the position of the first half edge via two horocycles [p_1,q_1] and [p_2,q_2]
   */
   DomeVolumeVisitor(Graph<Directed>& G, DoublyConnectedEdgeList& dcel_, const Matrix<Rational>& first_halfedge_horo)
      : dome_graph(G)
      , dcel(dcel_)
      , gkz_vector(dcel.getNumVertices())
      , angleVec(dcel.angleVector())
      , num_visited(0)
   {
      layFirstEdge( first_halfedge_horo );
   }

   Int numVisited() const { return num_visited; }

   const Vector<Rational>& getGKZvector() const { return gkz_vector; }

   bool operator()(Int n) { return operator()(n, n); }

   /* The operator used by the BFS iterator. It visits a new triangle, updates the GKZ vector,
    * and discovers the two adjacent triangles, adding them to the dual graph queue.
    */
   bool operator()(Int n_from, Int n_to)
   {
      if (visited.contains(n_to)) return false;
      /*
        The node n_to was not viseted yet, therefore the corresponding triangle (its known halfedge)
        did not contribute to the GKZ vector. We extract the known half edge as well as the two known
        horocycles from the edge_map. Then we compute the position of the third horocycle, and put two
        new adjacent triangles (half edges) in the queue.
      */
      const DecoratedEdge& edge_pair = edge_map[n_to];
      Vector<Rational> horo_u = edge_pair.second[0];
      Vector<Rational> horo_v = edge_pair.second[1];
      const HalfEdge* uv = dcel.getHalfEdge(edge_pair.first);
      const HalfEdge* vw = uv->getNext();
      const HalfEdge* wu = vw->getNext();
      Int u_id = dcel.getVertexId( wu->getHead() );
      Int v_id = dcel.getVertexId( uv->getHead() );
      Int w_id = dcel.getVertexId( vw->getHead() );

      const Vector<Rational> horo_w = thirdHorocycle(horo_u, horo_v, uv->getLength(), vw->getLength(), wu->getLength());

      // Scaling of horocycles
      Rational u_scaling = 1 / angleVec[u_id];
      Rational v_scaling = 1 / angleVec[v_id];
      Rational w_scaling = 1 / angleVec[w_id];

      /*
        Calculate the volume of the skew prism over the triangle (u,v,w) with repect to weights (1,...,1).
        Add it to the GKZ vector.
      */
      const Vector<Rational> vol = volumeSummands(horo_u, horo_v, horo_w, u_scaling, v_scaling, w_scaling);
      gkz_vector[u_id] += vol[0];
      gkz_vector[v_id] += vol[1];
      gkz_vector[w_id] += vol[2];

      /*
        Add the two new half edges to the queue, update the edge_map and the dome_graph.
        Note that we change the sign of the second horocycle (head of wv and uw) since we want to guarantee a positive determinant.
      */
      const Matrix<Rational> M_wv = vector2row(horo_w) / -horo_v;
      const Matrix<Rational> M_uw = vector2row(horo_u) / -horo_w;

      DecoratedEdge edge_pair_wv{ dcel.getHalfEdgeId(vw->getTwin()), M_wv };
      DecoratedEdge edge_pair_uw{ dcel.getHalfEdgeId(wu->getTwin()), M_uw };

      Int wv_node_id = dome_graph.add_node();
      dome_graph.add_edge(n_to, wv_node_id);
      edge_map[wv_node_id] = edge_pair_wv;
      Int uw_node_id = dome_graph.add_node();
      dome_graph.add_edge(n_to, uw_node_id);
      edge_map[uw_node_id] = edge_pair_uw;

      visited += n_to;
      ++num_visited;
      return true;
   }


   /*
     Lay out the first half edge (index=0) in H^2, according to the horocycles given by the two
     rows of the matrix first_halfedge_horo.
   */
   void layFirstEdge(const Matrix<Rational>& M_edge)
   {
      DecoratedEdge edge_pair{ 0, M_edge };
      edge_map[0] = edge_pair;

      const Matrix<Rational> M_twin_edge = vector2row(M_edge[1]) / -M_edge[0];
      DecoratedEdge twin_edge_pair{ 1, M_twin_edge };

      const Int new_node_id = dome_graph.add_node();
      dome_graph.add_edge(0, new_node_id);
      edge_map[new_node_id] = twin_edge_pair;
   }

   /*
     Projects a vector from the hyperboloid to the half-sphere model.
     The isometry from horocycles {[p,q]} to the half sphere light cylinder is given by
     (p,q) -> 1/(p^2+q^2)(p^2-q^2,-2pq,1).
   */
   static
   Vector<Rational> projectToHalfSphere(const Vector<Rational>& v)
   {
      const Rational factor = 1 / (sqr(v[0]) + sqr(v[1]));
      Vector<Rational> v_halfsphere{ factor * (sqr(v[1]) - sqr(v[0])), 2*factor*v[0]*v[1], factor };
      return v_halfsphere;
   }

   /*
     Return a 3-vector containing the respective gkz summand for each vertex of the triangle uvw.
     The scaling of a height in the light cylinder: z(weight)=weight*z(1).
     Here u_scaling=1/u_weight, where u_weight is the angle sum around the cusp with index u.
   */
   static
   Vector<Rational> volumeSummands(const Vector<Rational>& u, const Vector<Rational>& v, const Vector<Rational>& w,
                                   const Rational& u_scaling, const Rational& v_scaling, const Rational& w_scaling)
   {
      // The area of the base triangle (u,v,w) is given by A=1/2*|det[[u_1,u_2,1],[v_1,v_2,1],[w_1,w_2,1]]|.
      const Matrix<Rational> M = vector2row(projectToHalfSphere(u)) / projectToHalfSphere(v) / projectToHalfSphere(w);
      const Rational area = abs(det(M.minor(All, sequence(0,2)) | ones_vector<Rational>(3))/2);

      const Vector<Rational> vol{ u_scaling * M(0, 2), v_scaling * M(1, 2), w_scaling * M(2, 2) };
      return area * vol;
   }
}; // end class dome volume visitor


/* this wraps a bfs iterator over the dual graph of the infinite triangulation.
 * it has a method to compute the gkz vector up to a specified depth.
 * the part of the graph that is computed already gets stored in memory.
 * TODO maybe we don't have to store the whole graph in order to iterate?
 */
class DomeBuilder {
   Graph<Directed> dual_tree; // part of the dual tree of the triangulation
   Int cur_depth;
   graph::BFSiterator< Graph<Directed>, graph::VisitorTag<DomeVolumeVisitor> > bfs_it;

public:

   // construct from a dcel and the horo matrix of the first edge
   DomeBuilder(graph::DoublyConnectedEdgeList& dcel, const Matrix<Rational>& first_halfedge_horo)
      : dual_tree(1)  // start with a one-node graph
      , cur_depth(0)
      , bfs_it(dual_tree, DomeVolumeVisitor(dual_tree, dcel, first_halfedge_horo), nodes(dual_tree).front())
   {}

   Int getDepth() const { return cur_depth; }

   // get the gkz vector up to a given depth
   Vector<Rational> computeGKZVector(Int depth)
   {
      cur_depth = depth;
      Int num_nodes = 3*(pow(2, depth)-1)+1;  // number of nodes of a binary tree with ternary root of given depth

      while (bfs_it.node_visitor().numVisited() < num_nodes)
         ++bfs_it;

      return bfs_it.node_visitor().getGKZvector();
   }
};

} }


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
