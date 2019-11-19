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

#ifndef POLYMAKE_GRAPH_MATCHINGS_H
#define POLYMAKE_GRAPH_MATCHINGS_H

#include "polymake/Graph.h"
#include "polymake/Array.h"
#include "polymake/Set.h"
#include "polymake/graph/graph_iterators.h"
#include "polymake/graph/hungarian_method.h"

// TODO better representation for cycle

namespace polymake { namespace graph {

   // This implementation for finding all perfect matchings in a bipartite graph
   // broadly follows the algorithm described in the paper:
   //
   // Takeaki UNO - Algorithms for Enumerating All Perfect, Maximum and Maximal
   // Matchings in Bipartite Graphs

   class PerfectMatchings {

      protected:
         Graph<Directed> D;
         int dim;
         Set<Array<int>> matchings;

         class CycleVisitor;

      public:
         PerfectMatchings() {}
         /* PerfectMatchings(const Graph<Directed>& graph) */
         /*    : D(graph) */
         /*    , dim(graph.nodes()/2) */
         /* { */
         /*    // TODO checks for: */
         /*    //    * D bipartite with n+n nodes */
         /*    //    * D encodes perf. matching */
         /* } */
         PerfectMatchings(const Graph<Undirected>& graph, const Array<int> M) 
            : dim(graph.nodes()/2)
         {
            // some sanity checks first:
            if (graph.nodes() % 2 != 0)
               throw std::runtime_error("Graph has odd number of nodes.");
            if (graph.has_gaps())
               throw std::runtime_error("Graph has gaps."); // TODO squeeze() instead?
            for (int i = 0; i < dim; i++) {
               /* for (int j = 0; j < dim; j++) { */
               /*    if (graph.edge_exists(i,j) || graph.edge_exists(i+dim,j+dim)) */
               /*       throw std::runtime_error("Graph not bipartite of the form {0..n-1}U{n..2n-1}."); */
               /* } */
               for (auto n = entire(graph.adjacent_nodes(i)); !n.at_end(); ++n) {
                  if (*n < dim)
                     throw std::runtime_error("Graph not bipartite of the form {0..n-1}U{n..2n-1}.");
               }
               for (auto n = entire(graph.adjacent_nodes(i + dim)); !n.at_end(); ++n) {
                  if (*n >= dim)
                     throw std::runtime_error("Graph not bipartite of the form {0..n-1}U{n..2n-1}.");
               }
            }
            for (int i = 0; i < M.size(); i++) {
               if (!graph.edge_exists(M[i]+dim,i))
                  throw std::runtime_error("M not a matching of the given graph.");
            }
            if (M.size() != dim)
               throw std::runtime_error("Matching not perfect.");
            // build D(G,M):
            Graph<Directed> dirgraph(graph.nodes());
            for (int i = 0; i < dim; i++) {
               for (auto n = entire(graph.adjacent_nodes(i)); !n.at_end(); ++n) {
                  if (M[i] + dim == *n)
                     dirgraph.add_edge(*n, i);
                  else
                     dirgraph.add_edge(i, *n);
               }
            }
            this->D = dirgraph;
         }
         /* PerfectMatchings(const Graph<Undirected>& graph) { */
         /* } */

         Set<Array<int>> get_matchings()
         {
            collect_matchings(D);
            return matchings;
         }

      protected:
         class CycleVisitor : public NodeVisitor<> {
            typedef NodeVisitor<> base_t;
            public:
               bool cycle_found;
               std::vector<int> cycle;
            protected:
               std::vector<int> parent; // encodes the search tree
               std::vector<int> child;
               Set<int> path_set; // nodes of the current branch of the search tree
               int path_head;
            public:
               CycleVisitor() {}
               CycleVisitor(const Graph<Directed>& Din)
                  : base_t(Din)
                  , cycle_found(false)
                  , cycle(Din.dim(), -1)
                  , parent(Din.dim(), -1)
                  , child(Din.dim(), -1)
                  , path_set()
                  , path_head(-1)
               {}
               void clear(const Graph<Directed>&) {}
               bool operator() (int start_node)
               {
                  if (cycle_found)
                     return false;
                  visited += start_node;
                  path_set.clear();
                  path_set += start_node;
                  path_head = start_node;
                  return true;
               }
               bool operator() (int n_from, int n_to)
               {
                  if (cycle_found)
                     return false;
                  if (path_set.contains(n_to) && path_head == n_from) { // cycle found
                     cycle[0] = n_to;
                     for (int i = n_to, k = 1; i != n_from; i = child[i], k++) {
                        cycle[k] = child[i];
                     }
                     cycle_found = true;
                     return false;
                  } else if (visited.contains(n_to)) {
                     return false;
                  } else {
                     while (path_head != n_from) { // deal with (potential) branching in the search tree
                        path_set -= path_head;
                        path_head = parent[path_head];
                     }
                     path_set += n_to;
                     path_head = n_to;
                  }

                  parent[n_to] = n_from;
                  child[n_from] = n_to;
                  visited += n_to;

                  return true;
               }
         };

         std::vector<int> find_cycle(const Graph<Directed>& graph)
         {
            DFSiterator<Graph<Directed>, VisitorTag<CycleVisitor>> iter(graph);
            for (int i = 0; i < dim; i++) { // dfs per strongly connected component
               if (iter.node_visitor().get_visited_nodes().contains(i))
                  continue;
               iter.reset(i);
               while (!iter.at_end()) {
                  ++iter;
                  if (iter.node_visitor().cycle_found) {
                     return iter.node_visitor().cycle;
                  }
               }
            }
            return std::vector<int>();
         }

         Graph<Directed> augment(const Graph<Directed>& graph, std::vector<int> cycle)
         {
            Graph<Directed> G(graph);
            for (unsigned int i = 0; (i < cycle.size()) && cycle[i] >= 0; i++) {
               int n_to = i+1 < cycle.size() && cycle[i+1] >= 0 ? cycle[i+1] : cycle[0];
               G.delete_edge(cycle[i], n_to);
               G.add_edge(n_to, cycle[i]);
            }
            return G;
         }

         Array<int> extract_matching(const Graph<Directed>& graph)
         {
            Array<int> matching(dim, -1);
            for (int i = 0; i < dim; i++)
               matching[i] = graph.in_adjacent_nodes(i).front() - dim;
            return matching;
         }

         void collect_matchings(const Graph<Directed>& graph)
         {
            // TODO trim unnecessary edges

            std::vector<int> c = find_cycle(graph);

            if (c.empty()) {
               matchings += extract_matching(graph);
            } else {
               // choose matching edge:
               int start_index = c[0] > c[1] ? 0 : 1;
               std::pair<int,int> e(c[start_index], c[start_index+1]);

               // build graph G1 = G\(E-adjacent edges):
               Graph<Directed> g1(graph);
               int tmp;
               for (auto n = entire(g1.in_adjacent_nodes(e.first)); !n.at_end();) {
                  tmp = *n;
                  ++n;
                  g1.delete_edge(tmp, e.first);
               }
               for (auto n = entire(g1.out_adjacent_nodes(e.second)); !n.at_end();) {
                  tmp = *n;
                  ++n;
                  g1.delete_edge(e.second, tmp);
               }

               // build graph G2 = augment(G)\E:
               Graph<Directed> g2(augment(graph, c));
               g2.delete_edge(e.second, e.first);

               collect_matchings(g1);
               collect_matchings(g2);
            }
         }
   };

} }

#endif // POLYMAKE_GRAPH_MATCHINGS_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
// vim:shiftwidth=3:softtabstop=3:
