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
#include "polymake/Set.h"
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/graph/DoublyConnectedEdgeList.h"
#include "polymake/topaz/DomeVolumeVisitor.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Graph.h"
#include "polymake/Vector.h"
#include "polymake/graph/graph_iterators.h"
#include <cmath>


namespace polymake { namespace topaz {

using DoublyConnectedEdgeList = graph::dcel::DoublyConnectedEdgeList;
using HalfEdge = graph::dcel::HalfEdge;
using Face = graph::dcel::Face;

class PotatoVisitor : public graph::NodeVisitor<> {
private:
   Graph<Directed>& dual_tree;

   // The triangulation of the surface.
   DoublyConnectedEdgeList& dcel;

   // The vertices of the covering triangulation.
   std::vector<Vector<Rational>> points;

   // The co-vertices of the covering triangulation.
   std::vector<Vector<Rational>> co_points;

   // Mapping vectors to their position in points
   Map<Vector<Rational>, Int> vertex_map;

   // The number of nodes in the dual graph.
   const Int num_nodes_depth;

   // The triangles of the triangulation, triplets of corresponding indices.
   Array<Set<Int>> triangles;

   // mapping: BFS_node_id -> (half_edge_id, [vec_tail,vec_head])
   using edge_pair_t = std::pair<Int, Matrix<Rational>>;
   Map<Int, edge_pair_t> edge_map;

   Int curr_num_nodes;

   // A count of the nodes that were visited already.
   Int num_visited;

public:

   // This is needed for the BFS-iterator to not just stop in depth one.
   static constexpr bool visit_all_edges = true;

   /*
     The PV is initialized with the given graph G (usually only one node for the first triangle (0,0->next,0->next->next) ),
     the triangulation dcel (containing the A-coordinates),
     and the depth we want to visit the dual tree.
   */
   PotatoVisitor(Graph<Directed>& G, DoublyConnectedEdgeList& dcel_, const Matrix<Rational>& first_two_vertices, Int dual_tree_depth)
      : dual_tree(G)
      , dcel(dcel_)
      , num_nodes_depth(3*((1L << dual_tree_depth)-1)+1)
      , triangles(num_nodes_depth)
      , curr_num_nodes(0)
      , num_visited(0)
   {
      firstTriangle(first_two_vertices);
   }

//adding the first triangle, first vectors (half_edge_0 tail and head) are rows of first_two_vertices
void firstTriangle(const Matrix<Rational>& first_two_vertices)
{
   const HalfEdge* uv = dcel.getHalfEdge(0);
   const HalfEdge* vu = uv->getTwin();
   const HalfEdge* vw = uv->getNext();
   const HalfEdge* wu = vw->getNext();
   const HalfEdge* uw = wu->getTwin();
   const Rational& a_uv = uv->getLength();
   const Rational& a_vu = vu->getLength();
   const Rational& a_uw = uw->getLength();
   const Rational& a_vw = vw->getLength();
   const Face* uvw = uv->getFace();
   const Rational& a_uvw = uvw->getDetCoord();

   const Vector<Rational> first_vec = first_two_vertices[0];
   const Vector<Rational> second_vec = first_two_vertices[1];
   addVertex(first_vec);
   addVertex(second_vec);

   const Vector<Rational> first_co_vec{ Rational(0), a_uv, a_uw/a_uvw };
   const Vector<Rational> second_co_vec{ a_vu, Rational(0), a_vw/a_uvw };
   addCoVertex(first_co_vec);
   addCoVertex(second_co_vec);

   const Matrix<Rational> M_edge = vector2row(first_vec) / second_vec;
   edge_map[0] = edge_pair_t(0, M_edge);

   const Int new_node_id = dual_tree.add_node();
   dual_tree.add_edge(0, new_node_id);
   const Matrix<Rational> M_twin_edge = vector2row(second_vec) / first_vec;
   edge_map[new_node_id] = edge_pair_t(1, M_twin_edge);

   curr_num_nodes += 2;
}

/*
The vertex vec becomes a new row of the matrix points.
The vertex map at vec is set to the corresponding index of the row in points (with index shift due to counting from 0)
*/
void addVertex(const Vector<Rational>& vec)
{
   vertex_map[vec] = points.size();
   points.push_back(vec);
}

void addCoVertex(const Vector<Rational>& co_vec)
{
   co_points.push_back(co_vec);
}


Int numVisited() const { return num_visited; }

bool operator()(Int n)
{
   return operator()(n, n);
}


// this is what happens when we call bfs++
bool operator()(Int n_from, Int n_to)
{
   if (visited.contains(n_to)) return false;
   /*
     The node n_to was not visited yet, therefore the corresponding triangle (its known halfedge) did not contribute.
     We extract the known half edge as well as the two known horocycles .
     Then we compute the position of the third horocycle, and put two new adjacent triangles (half edges) in the queue.
   */
   const edge_pair_t& edge_pair = edge_map[n_to];
   const Vector<Rational> vec_u = edge_pair.second[0];
   const Vector<Rational> vec_v = edge_pair.second[1];
   const HalfEdge* uv = dcel.getHalfEdge(edge_pair.first);
   const HalfEdge* vw = uv->getNext();
   const HalfEdge* wv = vw->getTwin();
   const HalfEdge* wu = vw->getNext();
   const HalfEdge* uw = wu->getTwin();
   const Rational& a_vw = vw->getLength();
   const Rational& a_uw = uw->getLength();
   const Rational& a_wv = wv->getLength();
   const Rational& a_wu = wu->getLength();
   const Face* uvw = uv->getFace();
   const Rational& a_uvw = uvw->getDetCoord();
   const Vector<Rational>& l_u = co_points[vertex_map[vec_u]];
   const Vector<Rational>& l_v = co_points[vertex_map[vec_v]];
   const Vector<Rational> vec_w = thirdVector(vec_u, vec_v, l_u, l_v, a_uw, a_vw, a_uvw);
   const Vector<Rational> co_vec_w = thirdCoVector(vec_u, vec_v, vec_w, a_wu, a_wv);
   addVertex(vec_w);
   addCoVertex(co_vec_w);

   triangles[n_to] = Set<Int>{ vertex_map[vec_u], vertex_map[vec_v], vertex_map[vec_w] };

   if (dual_tree.nodes() < num_nodes_depth) {
      // Add the two new half edges to the queue, update the edge_map and the dual_tree.
      // Note that we change the sign of the second horocycle (head of wv and uw) since we want to guarantee a positive determinant.

      const Matrix<Rational> M_wv = vector2row(vec_w) / vec_v;
      const Matrix<Rational> M_uw = vector2row(vec_u) / vec_w;

      const Int wv_node_id = dual_tree.add_node();
      dual_tree.add_edge(n_to, wv_node_id);
      edge_map[wv_node_id] = edge_pair_t(dcel.getHalfEdgeId(wv), M_wv);
      const Int uw_node_id = dual_tree.add_node();
      dual_tree.add_edge(n_to, uw_node_id);
      edge_map[uw_node_id] = edge_pair_t(dcel.getHalfEdgeId(uw), M_uw);

      curr_num_nodes += 2;
   }

   visited += n_to;
   ++num_visited;

   return true;
}

Matrix<Rational> getPoints() const
{
   return Matrix<Rational>(points);
}

const Array<Set<Int>>& getTriangles() const
{
   return triangles;
}

DoublyConnectedEdgeList& getDcel() const
{
   return dcel;
}

Vector<Rational> thirdVector(const Vector<Rational>& vec_u, const Vector<Rational>& vec_v, const Vector<Rational>& l_u,
                             const Vector<Rational>& l_v, const Rational& a_uw, const Rational& a_vw, const Rational& a_uvw)
{
   const Vector<Rational> u_cross_v{ vec_u[1]*vec_v[2] - vec_u[2]*vec_v[1],
                                     vec_u[2]*vec_v[0] - vec_u[0]*vec_v[2],
                                     vec_u[0]*vec_v[1] - vec_u[1]*vec_v[0] };
   const Matrix<Rational> A = vector2row(l_u) / l_v / u_cross_v;
   const Matrix<Rational> A_inv = inv(A);
   const Vector<Rational> b{ a_uw, a_vw, a_uvw };
   const Vector<Rational> third_vector = A_inv*b;
   if (third_vector[0]+third_vector[1]+third_vector[2] < 0)
      throw std::runtime_error("You should choose a different affine chart");
   return A_inv*b;
}

Vector<Rational> thirdCoVector(const Vector<Rational>& vec_u, const Vector<Rational>& vec_v, const Vector<Rational>& vec_w, const Rational& a_wu, const Rational& a_wv)
{
   const Matrix<Rational> A = vector2row(vec_w) / vec_u / vec_v;
   const Matrix<Rational> A_inv = inv(A);
   const Vector<Rational> b{ Rational(0), a_wu, a_wv };
   return A_inv*b;
}

}; // end class PotatoVisitor


class PotatoBuilder {
   Graph<Directed> dual_tree; //part of the dual tree of the triangulation
   Int cur_depth;
   graph::BFSiterator< Graph<Directed>, graph::VisitorTag<PotatoVisitor> > bfs_it;

public:

   // construct from a dcel and the horo matrix of the first edge
   PotatoBuilder(DoublyConnectedEdgeList& dcel, const Matrix<Rational>& first_two_vertices, Int depth_in)
      : dual_tree(1)  //start with a one-node graph
      , cur_depth(depth_in)
      , bfs_it(dual_tree, PotatoVisitor(dual_tree, dcel, first_two_vertices, depth_in), nodes(dual_tree).front()) {}

   Int getDepth() const { return cur_depth; }

   // get the covering triangulation up to a given depth
   BigObject computeCoveringTriangulation()
   {
      Int num_nodes = 3*((1L << cur_depth)-1)+1; //number of nodes of a binary tree with ternary root of given depth
      while (bfs_it.node_visitor().numVisited() < num_nodes) {
         ++bfs_it;
      }

      const Matrix<Rational> points = ones_vector<Rational>() | bfs_it.node_visitor().getPoints();
      const Array<Set<Int>> triangles = bfs_it.node_visitor().getTriangles();

      return BigObject("fan::PolyhedralComplex<Rational>",
                       "POINTS", points,
                       "INPUT_POLYTOPES", triangles);
   }

}; // end class PotatoBuilder


// Compute a finite part of the triangulation covering the convex RP^2 surface given by A-coordinates "a_coords" on the triangulation "dcel_data".
// The rows of the 2x2 matrix "first_two_vertices" are the vertices of the first edge in R^3 that covers the first edge.
// The triangulation is calculated up to depth "depth" in the dual spanning tree rooted at the first triangle.
// Set "lifted" true in order to produce the concrete decorated triangulation in R^3.
BigObject projective_potato(const Matrix<Int>& dcel_data, const Vector<Rational>& a_coords, const Matrix<Rational>& first_two_vertices,
                            Int depth, OptionSet options)
{
   const bool lifted = options["lifted"];
   DoublyConnectedEdgeList dcel(dcel_data);
   dcel.setAcoords(a_coords);
   PotatoBuilder pot(dcel, first_two_vertices, depth);
   BigObject triang = pot.computeCoveringTriangulation();
   if (lifted) return triang;

   const Matrix<Rational> points = triang.give("POINTS");
   const Matrix<Rational> scaled = dcel.normalize(points.minor(All, range_from(1)));
   return BigObject("fan::PolyhedralComplex", mlist<Rational>(),
                    "POINTS", ones_vector<Rational>() | scaled,
                    "INPUT_POLYTOPES", triang.give("MAXIMAL_POLYTOPES"));
}

InsertEmbeddedRule("REQUIRE_APPLICATION fan\n\n");
UserFunction4perl("# @category Producing other objects\n"
                  "# Computes the triangulated convex projective set that covers the convex RP^2 surface."
                  "# The latter is given by the DCEL data for the triangulation of the surface along with A-coordinates (one positive Rational for each oriented edge and each triangle)."
                  "# Obviously, we only can compute a finite part of the infinite covering triangulation"
                  "# @param Matrix<Int> DCEL_data"
                  "# @param Vector<Rational> A_coords"
                  "# @param Matrix<Rational> first_two_vertices at the moment has to be the Matrix with rows (1,0,0),(0,1,0)"
                  "# @param Int depth"
                  "# @option Bool lifted for producing the lifted triangulation in R^3 with vertices in the light cone"
                  "# @return fan::PolyhedralComplex<Rational>"
                  "# @example The following computes a covering triangulation of a once punctured torus up to depth 5:"
                  "# > $T1 = new Matrix<Int>([[0,0,2,3,0,1],[0,0,4,5,0,1],[0,0,0,1,0,1]]);"
                  "# > $p = projective_potato($T1,new Vector([1,1,1,1,1,1,2,2]),new Matrix([[1,0,0],[0,1,0]]),1);"
                  "# > print $p->VERTICES;"
                  "# | 1 1 0 0"
                  "# | 1 0 1 0"
                  "# | 1 0 0 1"
                  "# | 1 1 1 -1"
                  "# | 1 -1/5 2/5 4/5"
                  "# | 1 2/5 -1/5 4/5",
                  &projective_potato, "projective_potato($ $ $ $ {lifted => 0})");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
