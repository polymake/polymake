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

#include "polymake/graph/Lattice.h"
#include "polymake/graph/ShrinkingLattice.h"
#include "polymake/graph/Decoration.h"
#include "polymake/Array.h"
#include "polymake/topaz/morse_matching_tools.h"
#include <cassert>


namespace polymake { namespace topaz {
namespace morse_matching_tools {

/**@brief Exchange matching arcs on alternating paths
 * @param M     Morse matching in Hasse diagram
 * @param p     stores the DFS-tree edges
 * @param v     start node
 * @param v     end node
 * @param size  size of the matching
 */
void exchangePath(const graph::ShrinkingLattice<graph::lattice::BasicDecoration>& M, MorseEdgeMap& EM, const Array<Int>& p, Int u, Int v, Int& size)
{
   Int w = v;   // start node
   do
   {
      Int ww = p[w];
      if (M.edge_exists(w, ww)) {
         const bool r = EM(w, ww);
         EM(w, ww) = !r;
         if (r)
            --size;
         else
            ++size;
      } else {
         const bool r = EM(ww, w);
         EM(ww, w) = !r;
         if (r)
            --size;
         else
            ++size;
      }
      w = ww;
   }
   while (w != u);
}


bool checkAcyclicDFS(const graph::ShrinkingLattice<graph::lattice::BasicDecoration>& M, const MorseEdgeMap& EM, Array<Int>& marked, Int v, bool lower, Int base)
{
   // mark node v as touched
   marked[v] = base;

   // if @v is on the lower side of a level
   if (lower) {
      // check all out-edges (pointing up in the Hasse diagram)
      for (auto e = M.out_edges(v).begin(); !e.at_end(); ++e) {
         // if the arc is in the arc set, i.e. reversed (pointing down)
         if (EM[*e]) {
            const Int source = e.to_node();
            // if the source-node is in the active tree we found a cycle
            if (marked[source] == base) {
               return false;
            } else {
               // if source-node is unmarked recurse
               if (marked[source] < base) {
                  if (! checkAcyclicDFS(M, EM, marked, source, false, base))
                     return false;
               }
            }
         }
      }
   } else {
      // otherwise we are on the upper size of a level
      // check all in-edges (pointing down in the Hasse diagram)
      for (auto e = M.in_edges(v).begin(); !e.at_end(); ++e) {
         if (! EM[*e]) {
            const Int target = e.from_node();
            // if the target-node is in the active tree we found a cycle
            if (marked[target] == base) {
               return false;
            } else {
               // if target-node is unmarked recurse
               if (marked[target] < base) {
                  if (! checkAcyclicDFS(M, EM, marked, target, true, base) )
                     return false;
               }
            }
         }
      }
   }
   marked[v] = base+1;
   return true;
}


//-------------------------------------------------------------------------------------------------------------
//------------------------------------- Compute Maximum Forests -----------------------------------------------
//-------------------------------------------------------------------------------------------------------------


/**@brief Modified Prim algorithm to find a maximum forest in a graph
 * @param G        graph
 * @param p        map to store edge to predecessor
 * @param visited  visted map
 *
 * We do not care about weights, we just want to produce an arbitrary spanning forest.
 *
 * - visited = -1 :  node deleted
 * - visited = 0  :  node not visited
 * - visited = 1  :  node visited and not root in the forest
 * - visited = 2  :  node visited and root in the forest
 *
 * The map @a p stores the predecessor of a node. Its type must be
 * equal to the type stored on each edge. The map should provide
 * operator[].
 *
 * @todo Use concept checks to guarantee that the type stored on each edge is
 *       the same as the one store in the predecessor map.
 *
 * @see completeToBottomLevel(), completeToTopLevel()
 */

void findMaximumForest(const Graph<Undirected>& G, const EdgeMap<Undirected, Int>& EM, Array<Int>& p, Array<Int>& visited)
{
   // initialize visited:
   for (Int i = 0; i < G.nodes(); ++i)
      visited[i] = 0;

   // search for start vertex and then start BFS iteration
   for (Int start = 0; start < G.nodes(); ++start)
   {
      if (visited[start] == 0) {   // if vertex has not be seen so far
         visited[start] = 2;   // mark vertex as root
         std::deque<Int> Q;

         // initialize edges from start vertex
         for (auto e = G.out_edges(start).begin(); !e.at_end(); ++e) {
            const Int w = e.to_node();
            if (visited[w] == 0) {
               p[w] = EM[*e];
               Q.push_back(w);
            }
         }

         // start BFS iterations
         while (! Q.empty()) {
            const Int v = Q.front();
            Q.pop_front();
            visited[v] = 1;

            for (auto e = G.out_edges(v).begin(); !e.at_end(); ++e) {
               const Int w = e.to_node();
               if (visited[w] == 0) {
                  p[w] = EM[*e];
                  Q.push_back(w);
               }
            }
         }
      }
   }
}


/**@brief Modified Prim algorithm to find a maximum forest in a graph, first trying to included marked nodes
 * @param G        graph
 * @param marked   map to mark vertices
 * @param p        map to store edge to predecessor
 * @param visited  visted map
 *
 * This function is similar to findMaximumForest(), but first tries to include nodes that are
 * marked by @a marked.
 *
 * We do not care about weights, we just want to produce an arbitrary spanning forest.
 *
 * - visited = -1 :  node deleted
 * - visited = 0  :  node not visited
 * - visited = 1  :  node visited and not root in the forest
 * - visited = 2  :  node visited and root in the forest
 *
 * The map @a p stores the predecessor of a node. Its type must be
 * equal to the type stored on each edge. The map should provide
 * operator[].
 *
 * @todo Use concept checks to guarantee that the type stored on each edge is
 *       the same as the one store in the predecessor map.
 *
 * @see completeToBottomLevel(), completToTopLevel()
 */
void findMaximumForestMarked(const Graph<Directed>& G, const EdgeMap<Directed, Int>& EM, std::vector<bool>& marked, Array<Int>& p, Array<Int>& visited)
{
   // initialize visited:
   for (Int i = 0; i < G.nodes(); ++i)
      visited[i] = 0;

   // first check for marked nodes
   for (Int start = 0; start < G.nodes(); ++start) {
      if (visited[start] == 0 && marked[start]) {     // if loop is not visited
         visited[start] = 2;
         std::deque<Int> Q;

         // initialize edges from start vertex
         for (auto e = G.out_edges(start).begin(); !e.at_end(); ++e) {
            const Int w = e.to_node();
            if ( visited[w] == 0 )
            {
               p[w] = EM[*e];
               Q.push_back(w);
            }
         }

         // start BFS iterations
         while (! Q.empty()) {
            const Int v = Q.front();
            Q.pop_front();
            visited[v] = 1;

            for (auto e = G.out_edges(v).begin(); !e.at_end(); ++e) {
               const Int w = e.to_node();
               if (visited[w] == 0) {
                  p[w] = EM[*e];
                  Q.push_back(w);
               }
            }
         }
      }
   }

   // search for start vertex and then start BFS iteration
   for (Int start = 0; start < G.nodes(); ++start)
   {
      if (visited[start] == 0) {   // if vertex has not be seen so far
         visited[start] = 2;   // mark vertex as root
         std::deque<Int> Q;

         // initialize edges from start vertex
         for (auto e = G.out_edges(start).begin(); !e.at_end(); ++e) {
            const Int w = e.to_node();
            if (visited[w] == 0) {
               p[w] = EM[*e];
               Q.push_back(w);
            }
         }

         // start BFS iterations
         while (! Q.empty()) {
            const Int v = Q.front();
            Q.pop_front();
            visited[v] = 1;

            for (auto e = G.out_edges(v).begin(); !e.at_end(); ++e) {
               const Int w = e.to_node();
               if (visited[w] == 0) {
                  p[w] = EM[*e];
                  Q.push_back(w);
               }
            }
         }
      }
   }
}

void remove_matching_from_1_skeleton(const graph::ShrinkingLattice<graph::lattice::BasicDecoration>& M, MorseEdgeMap& EM)
{
   for (const auto n : M.nodes_of_rank(1))
      for (auto e = M.out_edges(n).begin(); !e.at_end(); ++e)
         EM[*e] = false;
}

Bitset collectCriticalFaces(const graph::ShrinkingLattice<graph::lattice::BasicDecoration>& M, const MorseEdgeMap& EM)
{
   const Int d = M.rank()-2;      // do not count empty face
   const Int n = M.nodes()-2;    // and top face

   // ensure space
   Bitset critical(n+1);

   // loop over all levels
   for (Int k = 0; k <= d; ++k) {
      // pass through all faces of dimension k
      for (const auto f : M.nodes_of_rank(k+1)) {
         bool isCritical = true;
         // if the dimension is larger than 0, we can look at in-arcs
         if (k > 0) // pass through all in-arcs
            for (auto e = M.in_edges(f).begin(); !e.at_end() && isCritical; ++e) 
               // if the arc is in the matching (i.e., reversed), f is not critical
               if (EM[*e]) 
                  isCritical = false;
         // if the dimension is smaller than d, we can look at out-arcs
         if (isCritical && k < d)  // pass through all out-arcs
            for (auto e = M.out_edges(f).begin(); !e.at_end() && isCritical; ++e) 
               // if the arc is in the matching (i.e., reversed), f is not critical
               if (EM[*e])  
                  isCritical = false;
         if (isCritical)
            critical += f;
      }
   }
   return critical;
}


void processAlternatingPaths(graph::ShrinkingLattice<graph::lattice::BasicDecoration>& M, MorseEdgeMap& EM, Int& size, Int bottomLevel, Int topLevel)
{
#if POLYMAKE_DEBUG
   const bool debug_print = get_debug_level() > 1;
#endif
   const Int n = M.nodes()-2;
   Int cnt = 0; // number of alternating paths

   // compute critical faces
   Bitset critical = collectCriticalFaces(M, EM);

   // find alternating paths
   Array<Int> marked(n+1);
   Array<Int> dfsTree(n+1);
   for (Int k = bottomLevel; k < topLevel; ++k) {
      // check only nodes at the upper level
      for (const auto u : M.nodes_of_rank(k+2)) {
         if (critical.contains(u)) {
            for (Int i = 0; i <= n; ++i) {
               dfsTree[i] = -1;
               marked[i] = 0;
            }

            findAlternatingPathDFS(M, EM, marked, dfsTree, u, false);

            // check whether we found a path: lower part
            for (const auto v : M.nodes_of_rank(k+1)) {
               // if ( (v != u) && critical.contains(v) && (marked[v] == 1) )
               if (critical.contains(v) && (marked[v] == 1)) {
                  // check whether path contains only nodes visited once
                  Int w = v;
                  do {
                     w = dfsTree[w];
                     assert( w >= 0 );
                     if ( marked[w] != 1 )
                        break;
                  }
                  while (w != u);

                  if (w == u) {
                     // exchange path
                     exchangePath(M, EM, dfsTree, u, v, size);
                     ++cnt;

                     critical -= u;  // remove u and v
                     critical -= v;
                     break;
                  }
               }
            }
         }
      }
   }
#if POLYMAKE_DEBUG
   if (debug_print) cout << "Changed alternating paths: " << cnt << endl;
#endif
}


void findAlternatingPathDFS(const graph::ShrinkingLattice<graph::lattice::BasicDecoration>& M, const MorseEdgeMap& EM, Array<Int>& marked, Array<Int>& p, Int v, bool lower)
{
   marked[v] = 1;
   if (lower) {   // if we are one the lower side of a level
      for (auto e = M.out_edges(v).begin(); !e.at_end(); ++e) {
         if (EM[*e]) {
            Int u = e.to_node();
            if (marked[u] == 0) {
               p[u] = v;
               findAlternatingPathDFS(M, EM, marked, p, u, false);
            } else {
               ++(marked[u]);
            }
         }
      }
   } else {
      for (auto e = M.in_edges(v).begin(); ! e.at_end(); ++e) {
         if (! EM[*e]) {
            Int u = e.from_node();
            if (marked[u] == 0) {
               p[u] = v;
               findAlternatingPathDFS(M, EM, marked, p, u, true);
            } else {
               ++(marked[u]);
            }
         }
      }
   }
}


void make_edges_in_Gamma(const graph::ShrinkingLattice<graph::lattice::BasicDecoration>& M, const MorseEdgeMap& EM,
                         const Map<Int, Int>& FTON, Graph<Undirected>& Gamma, EdgeMap<Undirected, Int>& edge_map_Gamma)
{
   for (const auto f : M.nodes_of_rank(2)) { // iterate over all 1-faces of the complex
      bool is_unmatched_edge (true);
      for (auto e = M.out_edges(f).begin(); !e.at_end() && is_unmatched_edge; ++e) { 
         assert(M.rank(e.to_node()) == 3); // Assuming that out_edges go to the 2-skeleton
         if (EM[*e]) 
            is_unmatched_edge = false; // if a 1-face is matched to a 2-face, it doesn't count
      }
      if (is_unmatched_edge) {
         const auto& e = M.in_adjacent_nodes(f);
         assert(e.size()==2);
         const Int v1 = e.front(), v2 = e.back();

         Gamma.edge(FTON[v1], FTON[v2]);
         edge_map_Gamma(FTON[v1], FTON[v2]) = f;
      }
   }
}

void completeToBottomLevel(graph::ShrinkingLattice<graph::lattice::BasicDecoration>& M, MorseEdgeMap& EM)
{
   // find critical faces of best_solution_
   Bitset critical = collectCriticalFaces(M, EM);
#if POLYMAKE_DEBUG
   const bool debug_print = get_debug_level() > 1;

   if (debug_print) {
      cout << "critical faces: " << critical << endl;
      for (auto c = entire(critical); !c.at_end(); ++c)
         cout << M.face(*c) << " ";
      cout << endl;
   }
#endif

   // build helper graph
   Graph<Undirected> Gamma;
   NodeMap<Undirected, Int> node_map_Gamma(Gamma);
   EdgeMap<Undirected, Int> edge_map_Gamma(Gamma);

   // to translate between face indices and nodes of the graph:
   Map<Int, Int> FTON;

   // create nodes of the graph ( = vertices (0-faces) of complex )
   for (const auto f : M.nodes_of_rank(1)) {
      const Int v = Gamma.add_node();  // add 1-face
      node_map_Gamma[v] = f;
      FTON[f] = v;
   }

   // create edges (arising from 1-faces in the complex)
   make_edges_in_Gamma(M, EM, FTON, Gamma, edge_map_Gamma);

#if POLYMAKE_DEBUG
   if (debug_print) {
      cout << "Gamma: " << Gamma << endl
           << "edge_map_Gamma: " << edge_map_Gamma << endl;
   }
#endif

   // check whether graph is really connected (not strictly necessary!)
#if POLYMAKE_DEBUG
   if (debug_print) cout << "is graph connected: " << graph::is_connected(Gamma) << endl;
#endif
   assert( graph::is_connected(Gamma) );

   // prepare computation of spanning forest
   const Int numNodes = Gamma.nodes();
   Array<Int> visited(numNodes);
   Array<Int> p(numNodes);

   findMaximumForest(Gamma, edge_map_Gamma, p, visited);

#if POLYMAKE_DEBUG
   if (debug_print) {
      cout << "visited: " << visited << endl
           << "p: " << p << endl
           << "faces of p: ";
      for (Int i = 0; i < numNodes; ++i)
         cout << M.face(p[i]) << " ";
      cout << endl;
   }
#endif

   remove_matching_from_1_skeleton(M, EM);

   // now add corresponding arcs to the matching
   for (Int i = 0; i < numNodes; ++i) {
      assert( visited[i] != 0);
      // if vertex is in the forest, but not the root
      if (visited[i] == 1) {
         const Int fnum = p[i];       // index of 1-face in complex
         const Int vnum = node_map_Gamma[i];  // index of 0-face in complex
         EM(vnum, fnum) = true;
      }
   }
}

void completeToTopLevel(const graph::ShrinkingLattice<graph::lattice::BasicDecoration>& M, MorseEdgeMap& EM)
{
   const Int d = M.rank()-2;

   // find critical faces of best_solution_
   Bitset critical = collectCriticalFaces(M, EM);
   
   // build helper graph
   Graph<Directed> G;
   NodeMap<Directed, Int> node_map_G(G);
   EdgeMap<Directed, Int> edge_map_G(G);

   // to translate between face indices and nodes of the graph:
   Map<Int, Int> FTON;

   // to store loops
   std::vector<bool> loop;

   // create nodes of the graph ( = vertices (d-faces) of complex )
   for (const auto f : M.nodes_of_rank(d+1)) {
      const Int v = G.add_node();  // add d-face
      node_map_G[v] = f;
      FTON[f] = v;
      loop.push_back(false);   // assume that node numbers a consecutive
   }

   // create edges (arising from 1-faces in the complex)
   for (const auto f : M.nodes_of_rank(d)) {
      if (critical.contains(f)) {
         // find the two nodes incident to the edge
         Int v1 = -1;
         Int v2 = -1;
         for (auto e = M.out_edges(f).begin(); !e.at_end(); ++e) {
            if (v1 == -1) {
               v1 = e.to_node();
            } else {
               assert( v2 == -1);
               v2 = e.to_node();
            }
         }
         assert(v1 != -1);

         // either produce an edge or a loop:
         if (v2 != -1) {
            G.edge(FTON[v1], FTON[v2]);
            edge_map_G(FTON[v1], FTON[v2]) = f;
         } else {
            loop[FTON[v1]] = true;   // store that the d-face corr. to a loop
         }
      }
   }

   // prepare computation of spanning forest
   const Int numNodes = G.nodes();
   Array<Int> visited(numNodes);
   Array<Int> p(numNodes);

   findMaximumForestMarked(G, edge_map_G, loop, p, visited);

   // now add corresponding arcs to the matching
   for (Int i = 0; i < numNodes; ++i) {
      assert(visited[i] != 0);
      // if vertex is in the forest or the root and a loop
      if (visited[i] == 1 || (visited[i] == 2 && loop[i])) {
         Int rnum = p[i];       // index of (d-1)-face in complex
         Int fnum = node_map_G[i];  // index of d-face in complex
         EM(rnum, fnum) = true;
      }
   }
}

} } }

// Local Variables: 
// mode:C++ 
// c-basic-offset:3 
// indent-tabs-mode:nil 
// End: 
