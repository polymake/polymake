/* Copyright (c) 1997-2014
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

#ifndef POLYMAKE_HUNGARIAN_METHOD_H
#define POLYMAKE_HUNGARIAN_METHOD_H
#undef DOMAIN

#include "polymake/client.h"
#include "polymake/Graph.h"
#include "polymake/Set.h"
#include "polymake/Vector.h"
#include "polymake/Matrix.h"
#include "polymake/graph/BFSiterator.h"

namespace polymake { namespace graph {


template <typename E>
class HungarianMethod;

template <typename E>
class HungarianMethod {
protected:
   const Matrix<E> weights;
   const int dim;

   class TreeGrowVisitor;
   friend class TreeGrowVisitor;

   Vector<E> a, b, slack, labeledColMin;
   Graph<Directed> equality_subgraph;
   Set<int> exposed_points;
   Entire<Set<int> >::const_iterator r; /* an iterator over the exposed points */
   int start_node;
   BFSiterator< Graph<Directed>, Visitor< TreeGrowVisitor > > it; /* helps growing the hungarian trees from a start node */
   Graph<Directed> test_graph;
   Matrix<E> wmatrix;
   Set<int> labeled_points;

public: 
   HungarianMethod() {}
    
   HungarianMethod(const Matrix<E>& weights)  
      : weights(weights), dim(weights.cols()),
        a(dim,0), b(dim), 
        slack(dim,-1), labeledColMin(dim,-1), equality_subgraph(2*dim), 
        exposed_points(sequence(0,dim)), r(entire(exposed_points)), 
        start_node(*r), it(equality_subgraph, start_node)
   {        
      // Initialisation of vectors for rows and columns
      for ( int j = 0; j < dim; ++j) {
         b[j] = accumulate(weights.col(j), operations::min());
      }

      // Build equality subgraph; it is a bipartite graph with vertex sets {0, ..., n-1} and {n, ..., 2n-1}          
      for ( int j = 0; j < dim; ++j) {
         for ( int i = 0; i < dim; ++i) {
            if ((a[i] + b[j]) == weights[i][j]) {
               equality_subgraph.add_edge(i, dim + j);
            }
         }
      }

   }

   // This nested class provides the appropriate methods for the required BFS to grow hungarian trees
protected:
   class TreeGrowVisitor {
      friend class HungarianMethod;
   protected:
      // label encodes the path in the hungarian trees from an exposed point to the selected node
      std::vector<int> label;
      std::vector<bool> visited;
      int leaf;
      const int dim;
      const Graph<Directed>* H;
      Set<int > start_nodes;
   public:
      TreeGrowVisitor() {}

      TreeGrowVisitor(const Graph<Directed>& G, int start_node)
         : label(G.top().dim(), -1), visited(G.top().dim(), false), 
           dim((G.top().dim() + 1)/2), H(&G), start_nodes()
      {
         leaf = -1;
         start_nodes += start_node;
         if (!label.empty()){
            label[start_node] = start_node;
            visited[start_node] = true;
         }
      }
         
      // This method provides two functionalities: If the search had found a augmenting path and hence the equality subgraph was modified, it initializes a new search. Otherwise only another starting node for the growing of an hungarian tree is set.
      void reset(const Graph<Directed>&, int start_node) {
         if( (start_nodes.collect(start_node)) || (leaf > -1) ) reset_values();
         leaf = -1;
         label[start_node] = start_node;
         visited[start_node] = true;
         start_nodes +=start_node;
      }

      void reset_values() {
         start_nodes.clear();
         fill(pm::entire(label),-1);
         fill(pm::entire(visited),false);
      }

      bool seen(int n) const { return visited[n]; }

      void add(int n, int n_from) {
         visited[n] = true;
         // here the label is set during a BFS step in the equality subgraph 
         if(H->edge_exists(n_from, n)) { 
            label[n] = n_from; 
         } 
         // checking, if n is a leaf and is on the right side and so the path to n is augmenting 
         if ((n >= dim) && (H->out_degree(n) == 0)) leaf = n;
      } 

      static const bool check_edges=false;

      void check(int,int) {}

      const int& operator[] (int n) const { return label[n]; }

   };

public:
   // removes in the directed graph G the directed edge with starting point 'start' and end point 'end' and adds the reverse edge
   void reverse_edge (int start, int end) {
      assert(start >= 0 && end >= 0);
      equality_subgraph.delete_edge(start,end);
      equality_subgraph.add_edge(end,start);
   }


   /* searches in the equality subgraph for an augmenting path starting with the exposed point 'start_node' 
      returns true, if such a path is found, and augments the matching by reversing edges in the equality subgraph. */
   bool augment() {
      int node = it.node_visitor().leaf;
      int predecessor;
      // Going backwards in the hungarian tree from the leaf which was just found.  
      while (node != start_node) {
         predecessor = it.node_visitor()[node];
         // modifies the equality_subgraph, so that a new matching is encoded. 
         reverse_edge(predecessor, node);                 
         node = predecessor;               
      }
      // remove the start_node from exposed_points since it is matched now. 
      exposed_points -= start_node;
      // reset iterator over exposed_points 
      r = entire(exposed_points);
      // reset slack to -1 since the equality subgraph has changed 
      fill(pm::entire(slack),-1);
      fill(pm::entire(labeledColMin),-1); 
      
      // initialize the growing of hungarian trees if there are still exposed points left 
      if (!r.at_end()) {  
         it.reset(*r);
         return false;
      }
      else { return true; }
   }

   // checks for every right node (dual variable b) if the corresponding slack should be adjusted with the left node 'index' (dual variable a); the lowest value is chosen for it.
   void compare_slack(int index) { 
      E sl;
      for (int k = 0; k < b.dim(); k++) {
         sl = weights[index][k] - a[index] - b[k];
         if((sl < slack[k] || slack[k] == -1 || slack[k] == 0)) {
            if(sl > 0) {
               slack[k] = sl; 
               if (labeledColMin[k] != 0) {
                  labeledColMin[k] = sl;
               }
            }
         }
         if(sl == 0) labeledColMin[k] = 0;
      }
   }

   // auxiliary method for compare_slack
   void change_slack(int n) {
      if (n == start_node) compare_slack(n);               
      if  (n >= dim) {                     
         for (Entire<Graph<Directed>::out_edge_list>::const_iterator e=entire(equality_subgraph.out_edges(n)); !e.at_end(); ++e)
            compare_slack(e.to_node());               
      }  
   }

   // here the dual variables a and b are changed in case that the equality subgraph does not contain enough edges and so a maximal matching is not perfect
   void modify() {
      E theta = -1;
      // theta is the lowest positive value of the weights[i][j], where i corresponds to labeled left nodes and j runs from 0 to dim-1 
      for (int k = 0; k < dim; k++) {
         if ((slack[k] > 0) && ((slack[k] < theta) || (theta == -1) ) ) theta = slack[k]; 
      }
      for ( int k = 0; k < dim; k++) 
         if (it.node_visitor()[k] != -1) a[k] = a[k] + theta;

      for ( int k = 0; k < dim; k++) {
         if (labeledColMin[k] == 0 )
            b[k] = b[k] - theta; 
         for (int j = 0; j < dim; j++) {
            if ( (a[j] + b[k] != weights[j][k]) ) {
               equality_subgraph.delete_edge(j, k + dim);
               equality_subgraph.delete_edge(k + dim,j);
            }
         }
      }
      for ( int k = 0; k < dim; k++) {
         if( (labeledColMin[k] > 0) ) { // slack could also be -1 -- this is a symbolic infty 
            slack[k] = slack[k] - theta;
            if (slack[k] == 0) { // at least one new edge has been created at this point
               for (int j = 0; j < dim; j++) {
                  if (a[j] + b[k] == weights[j][k]) {
                     equality_subgraph.delete_edge(j,dim+k); //ensures that there are no multiple edges
                     equality_subgraph.add_edge(j,dim+k); 
                  }
               }
            }
            if (labeledColMin[k] > 0) labeledColMin[k] = slack[k];
         }
      }
      fill(pm::entire(slack),-1);
      fill(pm::entire(labeledColMin),-1);
      r = entire(exposed_points);
   }


   // initializes a bfs with start node start_node 
   int growTree () {
      it.reset(start_node);
      // search stops, if there is no further edge to go or a leaf of the hungarian tree is found, so that one can augment 
      while (!it.at_end() && (it.node_visitor().leaf == -1) ) {
         // the vector slack is adjusted, so that it contains the lowest values in the labeled rows when it is needed in the modification step 
         change_slack(*it);
         ++it;                           
      };
      return it.node_visitor().leaf;
   }
      
   // repeats the process of growing trees from exposed nodes until a perfect matching is found 
   Array<int> stage () {
      if (dim != 0) {
         bool finished = false;
         while (!finished) {
            while(!r.at_end()) {
               start_node = *r; 
               if (!(growTree() == -1)) finished = augment();
               else ++r;
            }
            if (!finished) {
               modify();
               it.reset(start_node);
               it.reset(*r);
            }
         }
      }

      Array<int > matching(dim) ;
      for (int k = 0; k < dim; k++) {
         matching[k] = equality_subgraph.in_adjacent_nodes(k).front() - dim;
      }
      return matching;
   }
};

} //end graph namespace
} //end polymake namespace

#endif // POLYMAKE_HUNGARIAN_METHOD_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
