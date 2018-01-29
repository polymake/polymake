/* Copyright (c) 1998-2016
Ewgenij Gawrilow, Michael Joswig (Technische Universitaet Berlin, Germainy)
http://www.polymake.org

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

#ifndef POLYMAKE_COVERING_TRIANGULATION_VISTOR_H
#define POLYMAKE_COVERING_TRIANGULATION_VISTOR_H

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

using namespace graph;

typedef std::list<int> flip_sequence;
/*
A decorated edge consists of the half edge index and a matrix for the two decorating horocycles. 
*/
typedef std::pair< int , Matrix<Rational> > DecoratedEdge;
/*
Each DecoratedEdge that appears in the BFS will get an unique index via this map.
*/
typedef Map< int , DecoratedEdge > EdgeMap;


class CoveringTriangulationVisitor : public NodeVisitor<> {

friend class DoublyConnectedEdgeList;

private: 


/*
The graph we want to iterate through. 
Gets build during iterating.
Part of the dual spanning tree of the triangulation of the universal covering H^2.
*/
Graph< Directed >& dual_tree;

/* 
The triangulation of the surface.
*/
DoublyConnectedEdgeList& dcel;

/* 
A map from the node-indices from the dual_tree to a pair (half edge , horocycles [[p_tail,q_tail],[p_head,q_head]]).
*/
EdgeMap edge_map;

/* 
The vertices of the triangulation.
*/
Matrix<Rational> points;

/*
Mapping horocycles to the row their corresponding vertices are of points
*/
Map< Vector<Rational> , int > vertex_map;

/*
The triangles of the triangulation, triplets of corresponding indices.
*/
Array< Set<int> > triangles;

/*
The number of nodes in the dual graph.
Is calculated in dependence of the variable dual_tree_depth.
*/
int num_nodes_depth;

int curr_num_nodes;

Vector<Rational> angleVec;

/*
A count of the nodes that were visited already.
*/
int num_visited;


public:

/* 
This is needed for the BFS-iterator to not just stop in depth one.
*/
static const bool visit_all_edges=true;

/*
The CTV gets initialized with the given graph G (usually only one node for the first half edge 0 / first triangle (0,0->next,0->next->next) ),
the triangulation dcel (containing the lambda lengths of the edges),
the position of the first half edge via two horocycles [[p_1,q_1] , [p_2,q_2]]
and the depth we want to visit the dual tree.
*/
CoveringTriangulationVisitor( Graph<Directed>& G, DoublyConnectedEdgeList& dcel, Matrix<Rational> first_halfedge_horo, int dual_tree_depth )
   : dual_tree(G)
   , dcel(dcel)
   , curr_num_nodes()
   , num_visited(0)
{
   angleVec = dcel.angleVector();
   layFirstEdge( first_halfedge_horo );
   num_nodes_depth = 3*( pow(2,dual_tree_depth) -1 )+1;
   triangles = Array< Set<int> >(num_nodes_depth);

}

int numVisited() const{ return num_visited; }

bool operator()( int n ) 
{
   return operator()( n , n );
}


bool operator()( int n_from , int n_to ) 
{
   if ( visited.contains( n_to ) ) return false;
   /*
   The node n_to was not viseted yet, therefore the corresponding triangle (its known halfedge) did not contribute to the GKZ vector.
   We extract the known half edge as well as the two known horocycles from the edge_map.
   Then we compute the position of the third horocycle, and put two new adjacent triangles (half edges) in the queue.
   */
   DecoratedEdge edge_pair = edge_map[n_to];
   Vector<Rational> horo_u = edge_pair.second[0];
   Vector<Rational> horo_v = edge_pair.second[1];
   HalfEdge uv = *dcel.getHalfEdge(edge_pair.first);
   HalfEdge vw = *(uv.getNext());
   HalfEdge wu = *(vw.getNext());
   Rational lambda_uv = uv.getLength();
   Rational lambda_vw = vw.getLength();
   Rational lambda_wu = wu.getLength();
   int u_id = dcel.getVertexId( wu.getHead() );
   int v_id = dcel.getVertexId( uv.getHead() );
   int w_id = dcel.getVertexId( vw.getHead() );

   Vector<Rational> horo_w = thirdHorocycle( horo_u , horo_v , lambda_uv , lambda_vw , lambda_wu );
//cout << "new horo = " << endl << horo_w << endl;
   
   Rational u_scaling = 1/angleVec[u_id];
   Rational v_scaling = 1/angleVec[v_id];
   Rational w_scaling = 1/angleVec[w_id];

   addVertex( horo_w , w_scaling );
//cout << "vertex map = " << endl << vertex_map << endl;

   Set<int> triangle;
   triangle += vertex_map[horo_u];
   triangle += vertex_map[horo_v];
   triangle += vertex_map[horo_w];
   triangles[n_to] = triangle;
//cout << "new triangle = " << endl << triangle << endl;

   if( dual_tree.nodes() < num_nodes_depth )
   {
      HalfEdge* wv = vw.getTwin();
      HalfEdge* uw = wu.getTwin();
      
      //Add the two new half edges to the queue, update the edge_map and the dual_tree.
      //Note that we change the sign of the second horocycle (head of wv and uw) since we want to guarantee a positive determinant.
      
      Matrix<Rational> M_wv(2,2); M_wv[0] = horo_w; M_wv[1] = -1*horo_v;
      Matrix<Rational> M_uw(2,2); M_uw[0] = horo_u; M_uw[1] = -1*horo_w;
      
      DecoratedEdge edge_pair_wv;
      edge_pair_wv.first = dcel.getHalfEdgeId(wv); edge_pair_wv.second = M_wv;
      DecoratedEdge edge_pair_uw;
      edge_pair_uw.first = dcel.getHalfEdgeId(uw); edge_pair_uw.second = M_uw;
      
      int wv_node_id = dual_tree.add_node();
      dual_tree.add_edge( n_to , wv_node_id );
      edge_map[wv_node_id] = edge_pair_wv;
      int uw_node_id = dual_tree.add_node();
      dual_tree.add_edge( n_to , uw_node_id );
      edge_map[uw_node_id] = edge_pair_uw;

      curr_num_nodes+=2;
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
void addVertex( Vector<Rational> horo_vector , Rational scaling )
{
   Rational p = horo_vector[0];
   Rational q = horo_vector[1];
   Rational a =  1/( p*p + q*q );
   Vector<Rational> v(3);
   v[0] = q*q - p*p; v[1] = 2*p*q;
   v[2] =  scaling/( p*p + q*q );
   v *= a;
   points/=v;
//cout << "points = " << endl << points << endl;
   vertex_map[ horo_vector ] = points.rows() - 1;
   vertex_map[ -1*horo_vector ] = points.rows() - 1;

}



/* 
Lay out the first half edge (index=0) in H^2, according to the input of the two horocycles (two rows of the matrix first_halfedge_horo). 
*/
void layFirstEdge( Matrix<Rational> horo_matrix )
{
   addVertex( horo_matrix[0] , 1/angleVec[0] );
   addVertex( horo_matrix[1] , 1/angleVec[1] );

   DecoratedEdge edge_pair;
   edge_pair.first = 0; edge_pair.second = horo_matrix;
   edge_map[0] = edge_pair;

   DecoratedEdge twin_edge_pair;
   Matrix<Rational> M_twin_edge(2,2);
   M_twin_edge[0] = horo_matrix[1]; M_twin_edge[1] = -horo_matrix[0];
   twin_edge_pair.first = 1; twin_edge_pair.second = M_twin_edge;

   int new_node_id = dual_tree.add_node();
   dual_tree.add_edge( 0 , new_node_id );
   edge_map[new_node_id] = twin_edge_pair;
   
   curr_num_nodes+=2;
}


Matrix<Rational> getPoints() const
{
   return points;
}

Array<Set<int>> getTriangles() const
{
   return triangles;
}


}; // end class covering triangulation volume visitor





class CoveringBuilder{
   Graph<Directed> dual_tree; //part of the dual tree of the triangulation
   int cur_depth;
   BFSiterator< Graph<Directed>, VisitorTag<topaz::CoveringTriangulationVisitor> > bfs_it;

   public:

   // construct from a dcel and the horo matrix of the first edge
   CoveringBuilder(DoublyConnectedEdgeList& dcel, Matrix<Rational> first_halfedge_horo, int depth_in):
     dual_tree(1), //start with a one-node graph
     cur_depth(depth_in),
     bfs_it(dual_tree, CoveringTriangulationVisitor(dual_tree, dcel, first_halfedge_horo, depth_in), nodes(dual_tree).front())
   {};

   int getDepth(){ return cur_depth; }

   // get the covering triangulation up to a given depth
   perl::Object computeCoveringTriangulation(){
      int num_nodes = 3*( pow(2,cur_depth) -1 )+1; //number of nodes of a binary tree with ternary root of given depth

      while(bfs_it.node_visitor().numVisited() < num_nodes)
      {
         ++bfs_it;
      }

      Matrix<Rational> points = bfs_it.node_visitor().getPoints();
      Array<Set<int>> triangles = bfs_it.node_visitor().getTriangles();

      perl::Object p("fan::PolyhedralComplex<Rational>");
      points = ones_vector<Rational>( points.rows() ) | points;
      p.take("POINTS") << points;
      p.take("INPUT_POLYTOPES") << triangles;

      return p;
   }

};




} //end topaz namespace


} //end polymake namespace
#endif // COVERING_TRIANGULATION_VISITOR_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:

