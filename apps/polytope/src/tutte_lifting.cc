/* Copyright (c) 1997-2015
   Ewgenij Gawrilow, Michael Joswig (Technische Universitaet Berlin, Germany)
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

#include "polymake/client.h"
#include "polymake/list"
#include "polymake/Set.h"
#include "polymake/Matrix.h"
#include "polymake/Graph.h"
#include "polymake/Array.h"
#include "polymake/Bitset.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Rational.h"
#include "polymake/linalg.h"
#include "polymake/Map.h"
#include <cmath>

namespace polymake { namespace polytope {

namespace {
typedef Map< Rational, int > AngleMap;
typedef Array< AngleMap > CycleList;
  
int getNext( const AngleMap& list, int node ) {
   // find the node
    
   bool found = false; 
   Entire< AngleMap >::const_iterator list_i = entire(list);
   while (! list_i.at_end() && !found) {
      found = list_i->second == node;
      ++list_i;
   }
   if ( list_i.at_end() ) ++list_i;     // wrap over to the begin
   return list_i->second;
}
  
Vector< Rational > crossProd(const Vector<Rational>& v1, const Vector<Rational>& v2) {
   Vector< Rational > result(4);
   result[0] = 0;
   result[1] = v1[2]*v2[3]-v1[3]*v2[2];
   result[2] = v1[3]*v2[1]-v1[1]*v2[3];
   result[3] = v1[1]*v2[2]-v1[2]*v2[1];
   return result;
}
}

perl::Object tutte_lifting(perl::Object g) {
   const Graph<> G=g.give("ADJACENCY");
   const int connectivity=g.give("CONNECTIVITY");
   if (connectivity < 3) {
      throw std::runtime_error("tutte_lifting: graph must be 3-connected");
   }
    
   const int n_nodes = G.nodes();
   const int n_facets = 2 - n_nodes + edges(G).size(); //EULER
  
   /*******************************
    * find a cycle of length 3
    ******************************/
   bool cycle_found = false;
   Bitset checked_verts(n_nodes); //remember them to not check them twice
   Set<int> act_cycle; //the actual cycle
   Set<int> inner_verts;
    
   for( int i=0; i < n_nodes && !cycle_found; ++i) {
      checked_verts += i;
      const Set<int> neigh_i = G.adjacent_nodes(i) - checked_verts;
      Set<int> checked_neighs;
    
      for( Entire< Set< int > >::const_iterator ni = entire(neigh_i); !ni.at_end() && !cycle_found; ++ni) {
         const Set<int> cycles = neigh_i * G.adjacent_nodes(*ni) - checked_neighs; 
         checked_neighs += *ni;
         for( Entire< Set< int > >::const_iterator ci = entire(cycles); !ci.at_end() && !cycle_found; ++ci) {
        
            act_cycle.clear();
            ( ( act_cycle += i) += *ci) += *ni;
            // check if the rest is connected via BFS
        
            Bitset visited(n_nodes);
            std::list<int> node_queue(n_nodes);
        
            for( Entire< Set< int > >::iterator vi = entire(act_cycle); !vi.at_end(); ++vi)
               visited += *vi;
        
            node_queue.clear();
            inner_verts.clear();
        
            // find starting node
            int start = 0;
            while( visited.contains(start) && start < n_nodes ) ++start;
        
            node_queue.push_back(start);
            visited += start;
            inner_verts += start;
        
            while (!node_queue.empty()) {
               const int n=node_queue.front();
               node_queue.pop_front();
               for (Entire<Graph<>::out_edge_list>::const_iterator e=entire(G.out_edges(n)); !e.at_end(); ++e) {
                  const int nn = e.to_node();
                  if (!visited.contains(nn)) {         // nn not been visited yet -> add nn to the queue.
                     visited += nn;
                     node_queue.push_back(nn);
                     inner_verts += nn;
                  }
               }
            }
            //check if all were visited
            int k = 0;
            while( k < n_nodes && visited.contains(k)) ++k;
            cycle_found = (k == n_nodes);
         }
      }
   }
  
   if (!cycle_found) {
      throw std::runtime_error("tutte_lifting: graph must contain a triangle");
   }
  
   /*******************************
    * create Stress Matrix and calculate 
    * Tutte-planar embedding
    *******************************/
   const int n_inner_verts = inner_verts.size();
  
   Matrix<Rational> stress_matrix(n_nodes,n_nodes);
   Matrix<Rational> V(n_nodes,4); //the VERTICES in R^3
   Matrix<Rational> fixed_coords(act_cycle.size(),4);
  
   // fix the outer triangle to (0,0),(1,0),(0,1)
   fixed_coords[1][1] = 1;
   fixed_coords[2][2] = 1;
   V.minor(act_cycle,All) = fixed_coords;
  
   Vector<Rational> rhs_x(n_inner_verts);
   Vector<Rational> rhs_y(n_inner_verts);
  
   int m=0;
   for (Entire< Set < int > >::const_iterator vi = entire(inner_verts); !vi.at_end(); ++vi,++m) {
      const Set<int> neigh_i = G.adjacent_nodes(*vi);
      stress_matrix[*vi][*vi] = - neigh_i.size();
      for (Entire< Set < int > >::const_iterator ni = entire(neigh_i); !ni.at_end(); ++ni) {
         if ( inner_verts.contains(*ni) ) {
            stress_matrix[*vi][*ni] = 1;          
         } else {
            rhs_x[m] -= V[*ni][1];
            rhs_y[m] -= V[*ni][2];
         }
      }
   }
  
   const Matrix<Rational> stress_matrix_inv = inv(stress_matrix.minor(inner_verts,inner_verts));
  
   V.minor(inner_verts,All).col(1) = stress_matrix_inv*rhs_x;
   V.minor(inner_verts,All).col(2) = stress_matrix_inv*rhs_y;
   V.col(0).fill(1);
   V.col(3).fill(1);
  
  
   // get all the neighbours in a cyclic way
   CycleList neigh_cyclic(n_nodes);
   sequence xy_coord=range(1,2);
  
   for ( int i = 0; i < n_nodes; ++i ) {
      const Set<int> neigh_i = G.adjacent_nodes(i);
      const Matrix<Rational> V_coords = V.minor(All,xy_coord);
    
      for (Entire< Set < int > >::const_iterator ni = entire(neigh_i); !ni.at_end(); ++ni) {
         Vector<Rational> ang_vec = V_coords[*ni] - V_coords[i];
         ang_vec /= (abs(ang_vec[0])+abs(ang_vec[1]));
         Rational angle = ((ang_vec[1] < 0 )?1 : -1)*(ang_vec[0]-1);
         neigh_cyclic[i][angle] = *ni;
      }
   }
  
   // CREATE THE DUAL GRAPH
   Graph<> DG(n_facets);
   IncidenceMatrix<> VIF(DG.nodes(),n_nodes);
   std::list< std::pair<int,int> > edge_queue(edges(G).size());
   edge_queue.clear();
  
   // push the outer triangle into the edge_queue
   Array< int > outer_verts(3);
   int l=0;
   for (Entire< Set < int > >::const_iterator ei = entire(act_cycle);!ei.at_end(); ++ei, ++l) {
      outer_verts[l]=*ei;
   }
  
   for (l=0; l < outer_verts.size(); ++l) {
      std::pair< int, int > edge(outer_verts[l%3],outer_verts[(l+1)%3]);    
      edge_queue.push_front(edge);
   }
  
   int facet_count = 0;
   VIF[0] = act_cycle;
   ++facet_count;
  
   while (!edge_queue.empty()) {
    
      std::pair< int, int > edge = edge_queue.front();
      edge_queue.pop_front();
    
      const Set< int > edge_in_facets = Set<int>(VIF.col(edge.first))*Set<int>(VIF.col(edge.second));
      if (edge_in_facets.size() == 2) { // the edge is contained in 2 known facets
         DG.edge(edge_in_facets.front(),edge_in_facets.back());
      } else { //edge is contained in 1 known facet
         Set< int > facet;
         const int start = edge.first;
         facet += start;
         int from = edge.first;
         int to = getNext(neigh_cyclic[start],edge.second);
         int next = -1;
         while (to != start) {
            edge.first = from;
            edge.second = to;
            edge_queue.push_back(edge);
            next = getNext(neigh_cyclic[to],from);
            facet += to;
            from  = to;
            to = next;
         }
         VIF[facet_count] = facet;
         DG.edge(edge_in_facets.front(),facet_count);// add edge in dual graph DG
         ++facet_count;
      }
   }
  
   //Calculate the cell centers
   Matrix<Rational> cell_centers(n_facets-1,2);
  
   for (int i = 0; i < n_facets-1; ++i) {
      cell_centers[i] = average(rows(V.minor(VIF[i+1],xy_coord)));
   }
  
   //CALCULATE THE HEIGHT OF THE POINTS
   //Calculate the facets
  
   Matrix< Rational > Facets(n_facets,4);
  
   std::list< int > facet_queue(n_facets);
   Set<int> facets_done;
   facets_done += 0;
   facets_done += 1;
   facet_queue.clear();
   facet_queue.push_back(1);
   facet_count--;
   int i;
   while (!facet_queue.empty() ) {
      i = facet_queue.front();
      facet_queue.pop_front();
      const Set< int > neigh_i = DG.adjacent_nodes(i) - facets_done;
    
      for (Entire< Set< int > >::const_iterator ni = entire(neigh_i); !ni.at_end(); ++ni) {
         // pts in i and *ni
         const Matrix< Rational > pts = V.minor(VIF[*ni]*VIF[i],All);
         // !!!!!!!!!! pay atttention to the direction of the edge !!!!!!!!!!
         const Vector< Rational > vp = pts[1].slice(xy_coord) - pts[0].slice(xy_coord);
         const Vector< Rational > vc = cell_centers[*ni-1]-cell_centers[i-1];
         const bool right_way = (vc[0]*vp[1]-vc[1]*vp[0] < 0);
         const Vector<Rational> cp = crossProd(pts.row(0),pts.row(1));
         Facets[*ni] = Facets[i] + (right_way?1:-1) * cp;
         --facet_count;
         facets_done += *ni;
         facet_queue.push_back(*ni);
      }
   }
  
   //CALCULATE THE HEIGHT
  
   for (int i = 0; i < n_nodes; ++i) {
      const Set< int > in_facets = VIF.col(i) - 0;
      V[i][3] = Facets[in_facets.front()]*V[i];
   }

   perl::Object p("Polytope<Rational>");
  
   p.take("VERTICES") << V;
   p.take("VERTICES_IN_FACETS") << VIF;
   p.take("DUAL_GRAPH.ADJACENCY") << DG;
   p.take("GRAPH.ADJACENCY") << G;
   p.take("GRAPH.CONNECTIVITY") << connectivity; 
   return p;
}

UserFunction4perl("# @category Producing a polytope from graphs"
                  "# Let //G// be a 3-connected planar graph. If the corresponding polytope"
                  "# contains a triangular facet (ie. the graph contains a non-"
                  "# separating cycle of length 3), the client produces a realization"
                  "# in R<sup>3</sup>."
                  "# @param Graph G"
                  "# @return Polytope"
                  "# @author Thilo Roerig",
                  &tutte_lifting,"tutte_lifting(Graph)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
