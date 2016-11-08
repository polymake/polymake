/* Copyright (c) 1997-2016
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

#include "polymake/client.h"
#include "polymake/Graph.h"
#include "polymake/Set.h"
#include "polymake/Vector.h"
#include "polymake/Matrix.h"
#include "polymake/graph/graph_iterators.h"

namespace polymake { namespace graph {

      //The implementation is adapted to Figure 11-2 in
      //Papadimitriou, Christos H.; Steiglitz, Kenneth
      //Combinatorial optimization: algorithms and complexity. Prentice-Hall, Inc., Englewood Cliffs, N.J., 1982. xvi+496 pp. ISBN: 0-13-152462-3 
      //
      //Corrections of the algorithms in this book can be found at
      //www.cs.princeton.edu/~ken/latest.pdf
      //
      //We fixed the algorithm independently of those errata.


template <typename E>
class HungarianMethod;

template <typename E>
class HungarianMethod {
protected:
   const Matrix<E> weights;
   const int dim;

   class TreeGrowVisitor;
   friend class TreeGrowVisitor;

   std::vector<E> a, b, slack, labeledColMin;
   Graph<Directed> equality_subgraph;
   Set<int> exposed_points;
   Entire<Set<int> >::const_iterator r;  // an iterator over the exposed points
   int start_node;
   BFSiterator<Graph<Directed>, VisitorTag<TreeGrowVisitor>> it; // helps growing the hungarian trees from a start node
   Graph<Directed> test_graph;
   Matrix<E> wmatrix;
   Set<int> labeled_points;

public: 
   HungarianMethod() {}
    
   HungarianMethod(const Matrix<E>& weights)  
      : weights(weights)
      , dim(weights.cols())
      , a(dim, 0)
      , b(dim)
      , slack(dim, -1)
      , labeledColMin(dim, -1)
      , equality_subgraph(2*dim)
      , exposed_points(sequence(0,dim))
      , r(entire(exposed_points))
      , start_node(*r)
      , it(equality_subgraph, start_node)
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
   class TreeGrowVisitor
      : public NodeVisitor<> {
      typedef NodeVisitor<> base_t;
      friend class HungarianMethod;
   protected:
      // label encodes the path in the hungarian trees from an exposed point to the selected node
      std::vector<int> label;
      int leaf;
      const int dim;
      const Graph<Directed>* H;
      Set<int> start_nodes;
   public:
      TreeGrowVisitor() {}

      TreeGrowVisitor(const Graph<Directed>& G)
         : base_t(G)
         , label(G.top().dim(), -1)
         , leaf(-1)
         , dim((G.top().dim() + 1)/2)
         , H(&G)
      {}
         
      void clear(const Graph<Directed>&) {}

      // This method provides two functionalities: If the search had found a augmenting path and hence the equality subgraph was modified, it initializes a new search. Otherwise only another starting node for the growing of an hungarian tree is set.
      bool operator()(int start_node)
      {
         if (start_nodes.contains(start_node) || leaf >= 0) {
            start_nodes.clear();
            std::fill(label.begin(), label.end(), -1);
            visited.clear();
            leaf = -1;
         }
         label[start_node] = start_node;
         visited += start_node;
         start_nodes += start_node;
         return true;
      }

      bool operator() (int n_from, int n_to)
      {
         if (visited.contains(n_to)) return false;

         visited += n_to;
         // here the label is set during a BFS step in the equality subgraph 
         if (H->edge_exists(n_from, n_to)) { 
            label[n_to] = n_from;
         } 
         // checking, if n is a leaf and is on the right side and so the path to n is augmenting 
         if ((n_to >= dim) && (H->out_degree(n_to) == 0)) leaf = n_to;
         return true;
      }

      const int& operator[] (int n) const { return label[n]; }
   };

public:
   // removes in the directed graph G the directed edge with starting point 'start' and end point 'end' and adds the reverse edge
   void reverse_edge (int start, int end)
   {
      assert(start >= 0 && end >= 0);
      equality_subgraph.delete_edge(start,end);
      equality_subgraph.add_edge(end,start);
   }


   /* searches in the equality subgraph for an augmenting path starting with the exposed point 'start_node' 
      returns true, if such a path is found, and augments the matching by reversing edges in the equality subgraph. */
   bool augment()
   {
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
      std::fill(slack.begin(), slack.end(), -1);
      std::fill(labeledColMin.begin(), labeledColMin.end(), -1); 
      
      // initialize the growing of hungarian trees if there are still exposed points left 
      if (!r.at_end()) {  
         it.reset(*r);
         return false;
      }
      else { return true; }
   }

   // checks for every right node (dual variable b) if the corresponding slack should be adjusted with the left node 'index' (dual variable a); the lowest value is chosen for it.
   void compare_slack(int index)
   { 
      E sl;
      for (size_t k = 0; k < b.size(); k++) {
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
   void change_slack(int n)
   {
      if (n == start_node) compare_slack(n);               
      if  (n >= dim) {                     
         for (Entire<Graph<Directed>::out_edge_list>::const_iterator e=entire(equality_subgraph.out_edges(n)); !e.at_end(); ++e)
            compare_slack(e.to_node());               
      }  
   }

   // here the dual variables a and b are changed in case that the equality subgraph does not contain enough edges and so a maximal matching is not perfect
   void modify()
   {
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
      std::fill(slack.begin(), slack.end(), -1);
      std::fill(labeledColMin.begin(), labeledColMin.end(), -1);
      r = entire(exposed_points);
   }


   // initializes a bfs with start node start_node 
   int growTree ()
   {
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
   Array<int> stage ()
   {
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

      Array<int> matching(dim) ;
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
