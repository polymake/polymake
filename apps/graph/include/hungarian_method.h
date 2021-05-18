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

namespace polymake { namespace graph {

      // The implementation is adapted to Figure 11-2 in
      // Papadimitriou, Christos H.; Steiglitz, Kenneth
      // Combinatorial optimization: algorithms and complexity. Prentice-Hall, Inc., Englewood Cliffs, N.J., 1982. xvi+496 pp. ISBN: 0-13-152462-3
      //
      // Corrections of the algorithms in this book can be found at
      // www.cs.princeton.edu/~ken/latest.pdf
      //
      // We fixed the algorithm independently of those errata.

template <typename E>
class HungarianMethod;

class TreeGrowVisitor : public NodeVisitor<> {
   using base_t = NodeVisitor<>;
   template <typename> friend class HungarianMethod;

   // label encodes the path in the hungarian trees from an exposed point to the selected node
   std::vector<Int> label;
   Int leaf;
   const Int dim;
   const Graph<Directed>* H;
   Set<Int> start_nodes;
public:
   TreeGrowVisitor(const Graph<Directed>& G)
      : base_t(G)
      , label(G.top().dim(), -1)
      , leaf(-1)
      , dim((G.top().dim()+1)/2)
      , H(&G)
   {}

   void clear(const Graph<Directed>&) {}

   // This method provides two functionalities: If the search had found a augmenting path and hence the equality subgraph was modified, it initializes a new search. Otherwise only another starting node for the growing of an hungarian tree is set.
   bool operator()(const Int start_node)
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

   bool operator() (Int n_from, Int n_to)
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

   Int operator[] (Int n) const { return label[n]; }
};

template <typename E>
class HungarianMethod {
protected:
   Matrix<E> weights;
   const Int dim;

   Vector<E> a, b, slack, labeledColMin;
   Graph<Directed> equality_subgraph;
   Set<Int> exposed_points;
   Set<Int>::const_iterator r;  // an iterator over the exposed points
   Int start_node;
   BFSiterator<Graph<Directed>, VisitorTag<TreeGrowVisitor>> it; // helps growing the hungarian trees from a start node
   /* Graph<Directed> test_graph; */
   Matrix<E> wmatrix;
   Set<Int> labeled_points;
   Array<Int> matching;
   const E INF = std::numeric_limits<E>::infinity();
   bool finished;
   bool inf_matching;

public:
   HungarianMethod() = default;

   explicit HungarianMethod(const Matrix<E>& input_weights)
      : dim(input_weights.cols())
      , a(dim, 0)
      , b(dim)
      , slack(dim, -1)
      , labeledColMin(dim, -1)
      , equality_subgraph(2*dim)
      , exposed_points(sequence(0,dim))
      , r(entire(exposed_points))
      , start_node(*r)
      , it(equality_subgraph, start_node)
      , matching(dim)
      , finished(false)
      , inf_matching(false)
   {

      // ensure that the weights are non-negative
      Vector<E> minvector(dim);
      auto minv = minvector.begin();
      for (auto wt = entire(rows(weights)); !wt.at_end(); ++wt, ++minv) {
         *minv = accumulate(*wt, operations::min());
      }
      weights = input_weights - repeat_col(minvector, dim);

      // Initialisation of vectors for rows and columns (dual variables)
      for (Int j = 0; j < dim; ++j) {
         b[j] = accumulate(weights.col(j), operations::min());
      }

      // Build equality subgraph; it is a bipartite graph with vertex sets {0, ..., n-1} and {n, ..., 2n-1}
      for (Int j = 0; j < dim; ++j) {
         for (Int i = 0; i < dim; ++i) {
            if ((a[i] + b[j]) == weights[i][j]) {
               equality_subgraph.add_edge(i, dim + j);
            }
         }
      }
   }

   // removes in the directed graph G the directed edge with starting point 'start' and end point 'end' and adds the reverse edge
   void reverse_edge (Int start, Int end)
   {
      assert(start >= 0 && end >= 0);
      equality_subgraph.delete_edge(start,end);
      equality_subgraph.add_edge(end,start);
   }


   /* searches in the equality subgraph for an augmenting path starting with the exposed point 'start_node'
      returns true, if such a path is found, and augments the matching by reversing edges in the equality subgraph. */
   bool augment()
   {
      Int node = it.node_visitor().leaf;
      Int predecessor;
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
      } else {
         return true;
      }
   }

   // checks for every right node (dual variable b) if the corresponding slack should be adjusted with the left node 'index' (dual variable a); the lowest value is chosen for it.
   void compare_slack(Int index)
   {
      E sl;
      for (Int k = 0; k < b.dim(); k++) {
         sl = weights[index][k] - a[index] - b[k];
         if((sl < slack[k] || slack[k] == -1 || slack[k] == 0)) {
            if(sl > 0) { //this excludes the case where sl == 0
               slack[k] = sl;
               if (labeledColMin[k] != 0) {
                  labeledColMin[k] = sl;
               }
            }
         }
         if(sl == 0) labeledColMin[k] = 0; // records that (k +dim) is indeed a matched node
      }
   }

   // auxiliary method for compare_slack
   void change_slack(Int n)
   {
      if (n == start_node) compare_slack(n);
      // sets new slack if n is a node on the right shore
      if  (n >= dim) {
         for (auto e=entire(equality_subgraph.out_edges(n)); !e.at_end(); ++e)
            compare_slack(e.to_node());
      }
   }

   // auxiliary method to find an infty entry in weights to create a minimizing permutation in the case that it is infty
   std::pair<Int, Int> inf_entry()
   {
      for (const auto i : sequence(0, dim)) {
         for (const auto j : sequence(0, dim)) {
            if (weights[i][j] == INF)
               return std::make_pair(i,j);
         }
      }
      throw std::runtime_error("no inf entry found but slack is inf; this happened due to an implementation error");
   }

   // here the dual variables a and b are changed in case that the equality subgraph does not contain enough edges and so a maximal matching is not perfect
   void modify()
   {
      E theta = -1;
      // theta is the lowest positive value of the weights[i][j], where i corresponds to labeled left nodes and j runs from 0 to dim-1
      for (Int k = 0; k < dim; k++) {
         if ((labeledColMin[k] > 0) && (slack[k] > 0) && ((slack[k] < theta) || (theta == -1)))
            theta = slack[k];
      }

      if (theta == INF) {
         finished = true;
         inf_matching = true;
         return;
      }

      for (Int k = 0; k < dim; ++k) {
         if (it.node_visitor()[k] != -1)
            a[k] = a[k] + theta;
      }

      for (Int k = 0; k < dim; ++k) {
         if (it.node_visitor()[k+dim] != -1) //(labeledColMin[k] == 0 )
            b[k] = b[k] - theta;
      }

      for (Int k = 0; k < dim; ++k) {
         for (Int j = 0; j < dim; ++j) {
            if (a[j] + b[k] != weights[j][k]) {
               equality_subgraph.delete_edge(j, k+dim);
               equality_subgraph.delete_edge(k+dim, j);
            }
         }
      }

      for (Int k = 0; k < dim; ++k) {
         if (labeledColMin[k] > 0) { // slack could also be -1 -- this is a symbolic infinity
            slack[k] = slack[k] - theta;
            if (slack[k] == 0) { // at least one new edge has been created at this point
               for (Int j = 0; j < dim; j++) {
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
   Int growTree ()
   {
      it.reset(start_node);
      // search stops, if there is no further edge to go or a leaf of the hungarian tree is found, so that one can augment
      while (!it.at_end() && (it.node_visitor().leaf == -1) ) {
         // the vector slack is adjusted, so that it contains the lowest values in the labeled rows when it is needed in the modification step
         change_slack(*it);
         ++it;
      }
      return it.node_visitor().leaf;
   }

   // repeats the process of growing trees from exposed nodes until a perfect matching is found
   void stage ()
   {
      if (dim != 0) {
         while (!finished) {
            /* print_stuff(0); */
            while(!r.at_end()) {
               start_node = *r;
               if (growTree() != -1)
                  finished = augment();
               else ++r;
            }
            if (!finished) {
               modify();
               if (!inf_matching) {
                  it.reset(start_node);
                  it.reset(*r);
               }
            }
         }

         if (!inf_matching) {
            for (Int k = 0; k < dim; k++) {
               matching[k] = equality_subgraph.in_adjacent_nodes(k).front() - dim;
            }
         } else {
            // inf matching recognized
            std::pair<Int, Int> trans = inf_entry();
            matching = Array<Int>(sequence(0, dim));
            matching[trans.first] = trans.second;
            matching[trans.second] = trans.first;
         }
      }
   }

   // TODO genericvector?
   void dynamic_stage(Int index, const Vector<E>& column)
   {
      // insert new column:
      weights.col(index) = column;

      // TODO man muss mit der non-negative-machung aufpassen! (glaube ich)
      // recalculate dual variable at 'index':
      b[index] = accumulate(column - a, operations::min());

      // adjust the eq-subgraph and exposed point(s):
      for (Int i = 0; i < dim; i++) { // TODO iterate over equality_subgraph.adjacent_nodes() instead?
         equality_subgraph.delete_edge(i, dim + index);
         equality_subgraph.delete_edge(dim + index, i);
         if (a[i] + b[index] == weights[i][index])
            equality_subgraph.add_edge(i, dim + index);
         if (matching[i] == index)
            exposed_points += i;
      }

      // reset iterator over exposed point(s):
      r = entire(exposed_points);

      /* print_stuff(1); */

      // one final stage of the hungarian algo:
      finished = false;
      stage();

      /* print_stuff(1); */

   }

   std::pair<Vector<E>, Vector<E>> get_cover()
   {
      return std::pair<Vector<E>, Vector<E>>(a, b);
   }

   const Array<Int>& get_matching()
   {
      return matching;
   }

   const Graph<Directed>& get_equality_subgraph()
   {
      return equality_subgraph;
   }

   E get_value()
   {
      if (inf_matching)
         return INF;
      else
         return (accumulate(a, operations::add()) + accumulate(b, operations::add()));
   }
};

} //end graph namespace
} //end polymake namespace


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
// vim:shiftwidth=3:softtabstop=3:
