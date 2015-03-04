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

   @author Marc Pfetsch
   @brief  tools for computations with Morse matchings
   @todo Check whether overflow can occur in @a base (see checkAcyclicDFS() ).
*/

#ifndef POLYMAKE_TOPAZ_MORSE_MATCHING_TOOLS_H
#define POLYMAKE_TOPAZ_MORSE_MATCHING_TOOLS_H

#include "polymake/client.h"
#include "polymake/graph/connected.h"
#include "polymake/Array.h"
#include "polymake/Map.h"
#include "polymake/PowerSet.h"
#include "polymake/graph/HasseDiagram.h"
#include "polymake/vector"
#include <cassert>
#include <deque>

namespace polymake { namespace topaz {

typedef EdgeMap<Directed, int> HasseEdgeMap;
typedef Graph<Directed>::in_edge_list::const_iterator HasseDiagramInConstIterator;
typedef Graph<Directed>::out_edge_list::const_iterator HasseDiagramOutConstIterator;



//-------------------------------------------------------------------------------------------------------------
//------------------------------------- Check Morse properties ------------------------------------------------
//-------------------------------------------------------------------------------------------------------------


/**@brief Recursive function to detect cycles arising from reversing the arcs in @a M.
 * @param M         collection of arcs in Hasse diagram
 * @param marked    marker for DFS-search (node has been visited)
 * @param v         node to be searched
 * @param lower     whether we are on the lower part
 * @param base      base value for marker (see below)
 * @returns @c true if the layer is acyclic
 *
 * We use a depth-first-search (DFS) to detect (directed) cycles in the Hasse diagram
 * where arcs whose value is @c true are reversed. Such cycles cannot span several
 * levels. We therefore search each level individually. The parameter @c lower stores
 * whether the current node @a v is on the lower or the upper part of the level.
 *
 * To avoid reinitialization of @c marked we use the following convention for the markers:
 * - if the marker is less than @c base, the node has not been visited
 * - if the marker is equal to @c base, the node is in the current active subtree
 * - if the marker is equal to @c base + 1, the node and all its children have been processed.
 *
 * If we find an arc from @a v to some node marked with @c base we found a cycle.
 * If its mark is less than @c base we recurse on this node. If the mark is @c base + 1
 * we found a node in a different part of the tree, which does not yield a cycle.
 *
 * This function is called, e.g. from @p checkAcyclic.
 */
//bool checkAcyclicDFS(const HasseDiagram<Set<int>,bool>& M, Array<int>& marked, int v, bool lower, int base)
template <class EdgeMap>
bool checkAcyclicDFS(const graph::HasseDiagram& M, const EdgeMap& EM, Array<int>& marked, int v, bool lower, int base)
{
   // mark node v as touched
   marked[v] = base;

   // if @v is on the lower side of a level
   if (lower)
   {
      // check all out-edges (pointing up in the Hasse diagram)
      for (HasseDiagramOutConstIterator e = M.out_edges(v).begin(); !e.at_end(); ++e)
      {
         // if the arc is in the arc set, i.e. reversed (pointing down)
         if (EM(e.from_node(), e.to_node()))
         {
            int source = e.to_node();
            // if the source-node is in the active tree we found a cycle
            if (marked[source] == base)
               return false;
            else
            {
               // if source-node is unmarked recurse
               if (marked[source] < base)
               {
                  if (! checkAcyclicDFS(M, EM, marked, source, false, base) )
                     return false;
               }
            }
         }
      }
   }
   else // otherwise we are on the upper size of a level
   {
      // check all in-edges (pointing down in the Hasse diagram)
      for (HasseDiagramInConstIterator e = M.in_edges(v).begin(); !e.at_end(); ++e)
      {
         if (! EM(e.from_node(), e.to_node()) )
         {
            int target = e.from_node();
            // if the target-node is in the active tree we found a cycle
            if (marked[target] == base)
               return false;
            else
            {
               // if target-node is unmarked recurse
               if (marked[target] < base)
               {
                  if (! checkAcyclicDFS(M, EM, marked, target, true, base) )
                     return false;
               }
            }
         }
      }
   }
   marked[v] = base + 1;
   return true;
}





/**@brief Check whether a given solution is acyclic
 * @param M         collection of arcs in Hasse diagram
 * @param marked    marker for DFS-search (node has been visited)
 * @param base      base value for marker (see @c checkAcyclicDFS())
 * @returns @c true if the solution is acyclic
 *
 * We assume that @c marked has been initialized some time ago.
 * @warning @a base is changed!
 */
template <class EdgeMap>
bool checkAcyclic(const graph::HasseDiagram& M, const EdgeMap& EM,  Array<int>& marked, int& base)
{
   int d = M.dim() - 1;      // do not count empty face and top face

   // check each level in turn
   for (int k = 0; k < d; ++k)
   {
      // increase base
      base += 2;

      // check each face of dim k
      for (Entire<graph::HasseDiagram::nodes_of_dim_set>::const_iterator f = entire(M.nodes_of_dim(k)); !f.at_end(); ++f)
      {
         int v = *f;
         assert( v > 0 && v <= (M.nodes()-2) );

         // for each unmarked face
         if ( marked[v] < base )
         {
            bool acyclic = checkAcyclicDFS(M, EM, marked, v, true, base);
            if (! acyclic)
               return false;
         }
      }
   }
   return true;
}




/**@brief Check whether a given solution is acyclic
 * @param M         collection of arcs in Hasse diagram
 * @param k         dimension of the level to be checked
 * @param marked    marker for DFS-search (node has been visited)
 * @param base      base value for marker (see @c checkAcyclicDFS())
 * @returns @c true if the solution is acyclic
 *
 * We assume that @c marked has been initialized some time ago.
 */
template <class EdgeMap>
bool checkAcyclic(const graph::HasseDiagram& M, const EdgeMap& EM, int k, Array<int>& marked, int base)
{
   // check each face of dim k
   for (Entire<graph::HasseDiagram::nodes_of_dim_set>::const_iterator f = entire(M.nodes_of_dim(k)); !f.at_end(); ++f)
   {
      int v = *f;
      assert( v > 0 && v <= (M.nodes()-2) );

      // for each unmarked face
      if ( marked[v] < base )
      {
         bool acyclic = checkAcyclicDFS(M, EM, marked, v, true, base);
         if (! acyclic)
            return false;
      }
   }
   return true;
}



/**@brief Check whether a given solution is acyclic
 * @param M  collection of arcs in Hasse diagram
 * @returns @c true if the solution is acyclic
 */
template <class EdgeMap>
bool checkAcyclic(const graph::HasseDiagram& M, const EdgeMap& EM)
{
   int d = M.dim() - 1;      // do not count empty face
   int n = M.nodes() - 2;    // and top face

   Array<int> marked(n+1);
   // initialize markers
   for (int i = 0; i <= n; ++i)
      marked[i] = 0;

   // for each level in turn
   int base = -1;
   for (int k = 0; k < d; ++k)
   {
      // increase base
      base += 2;

      // check each face of dim k
      for (Entire<graph::HasseDiagram::nodes_of_dim_set>::const_iterator f = entire(M.nodes_of_dim(k)); !f.at_end(); ++f)
      {
         const int v = *f;
         assert( (v > 0) && (v <= n) );

         // for each unmarked face
         if ( marked[v] < base )
         {
            if (!checkAcyclicDFS(M, EM, marked, v, true, base))
               return false;
         }
      }
   }
   return true;
}



/**@brief Check whether the given collection of arcs satisfies the matching constraints
 * @param M  collection of arcs in Hasse diagram
 * @returns @c true if the given arc set is a matching
 */
template <class EdgeMap>
bool checkMatching(const graph::HasseDiagram& M, const EdgeMap& EM)
{
#if POLYMAKE_DEBUG
   const bool debug_print = perl::get_debug_level() > 1;
#endif

   const int d = M.dim() - 1;

   // for each level in turn
   for (int k = 0; k <= d; ++k) {
      // check each face of dim k
      for (Entire<graph::HasseDiagram::nodes_of_dim_set>::const_iterator f = entire(M.nodes_of_dim(k)); !f.at_end(); ++f) {
         int inc = 0;  // count incident arcs
         if ( k > 0 ) {
            for (HasseDiagramInConstIterator e = M.in_edges(*f).begin(); !e.at_end(); ++e) {
               // if arc is in collection, i.e. reverse we have an incident arc
               if ( EM(e.from_node(), e.to_node()) )
                  ++inc;
               // if a node has more than two incident arcs the matching property is violated
               if (inc >= 2) {
#if POLYMAKE_DEBUG
                  if (debug_print) cout << "Matching propery violated for node " << *f << "=" << M.face(*f) << endl;
#endif
                  return false;
               }
            }
         }
         if ( k < d ) {
            for (HasseDiagramOutConstIterator e = M.out_edges(*f).begin(); !e.at_end(); ++e) {
               // if arc is in collection, i.e. reverse we have an incident arc
               if ( EM(e.from_node(), e.to_node()) )
                  ++inc;
               // if a node has more than two incident arcs the matching property is violated
               if (inc >= 2) {
#if POLYMAKE_DEBUG
                  if (debug_print) cout << "Matching propery violated for node " << *f << "=" << M.face(*f) << endl;
#endif
                  return false;
               }
            }
         }
      }
   }
   return true;
}







//-------------------------------------------------------------------------------------------------------------
//------------------------------------- Other functions -------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------


/**@brief Find the critical faces w.r.t the Morse matching in @a M
 * @param M         Morse matching in Hasse diagram
 * @returns bitset indicating the critical faces (nodes)
 *
 * On output an element (node) is set to true in the bitset if and only if the face is critical.
 */
template<class EdgeMap>
Bitset collectCriticalFaces(const graph::HasseDiagram& M, const EdgeMap& EM)
{
   const int d = M.dim() - 1;      // do not count empty face
   const int n = M.nodes() - 2;    // and top face

   // ensure space
   Bitset critical(n+1);

   // loop over all levels
   for (int k = 0; k <= d; ++k) {
      // pass through all faces of dimension k
      for (Entire<graph::HasseDiagram::nodes_of_dim_set>::const_iterator f = entire(M.nodes_of_dim(k)); !f.at_end(); ++f) {
         bool isCritical = true;
         // if the dimension is larger than 0, we can look at in-arcs
         if (k > 0) // pass through all in-arcs
            for (HasseDiagramInConstIterator e = M.in_edges(*f).begin(); !e.at_end() && isCritical; ++e) 
               // if the arc is in the matching (i.e., reversed), f is not critical
               if (EM(e.from_node(), e.to_node())) 
                  isCritical = false;
         // if the dimension is smaller than d, we can look at out-arcs
         if ( isCritical && k < d )  // pass through all out-arcs
            for (HasseDiagramOutConstIterator e = M.out_edges(*f).begin(); !e.at_end() && isCritical; ++e) 
               // if the arc is in the matching (i.e., reversed), f is not critical
               if (EM(e.from_node(), e.to_node()))  
                  isCritical = false;
         if ( isCritical )
            critical += *f;
      }
   }
   return critical;
}







/**@brief Find the critical faces w.r.t the Morse matching in @a M
 * @param M         Morse matching in Hasse diagram
 * @returns set containing critical faces
 */
template <class EdgeMap>
PowerSet<int> findCriticalFaces(const graph::HasseDiagram& M, const EdgeMap& EM)
{
   int d = M.dim() - 1;      // do not count empty face

   // ensure space
   PowerSet<int> critical;

   // loop over all levels
   for (int k = 0; k <= d; ++k)
   {
      // pass through all faces of dimension k
      for (Entire<graph::HasseDiagram::nodes_of_dim_set>::const_iterator f = entire(M.nodes_of_dim(k)); !f.at_end(); ++f)
      {
         bool isCritical = true;
         // if the dimension is larger than 0, we can look at in-arcs
         if (k > 0)
         {
            // pass through all in-arcs
            for (HasseDiagramInConstIterator e = M.in_edges(*f).begin(); !e.at_end(); ++e)
            {
               // if the arc is in the matching (i.e., reversed), f is not critical
               if (EM(e.from_node(), e.to_node()))
               {
                  isCritical = false;
                  break;
               }
            }
         }
         // if the dimension is smaller than d, we can look at out-arcs
         if ( isCritical && k < d )
         {
            // pass through all out-arcs
            for (HasseDiagramOutConstIterator e = M.out_edges(*f).begin(); !e.at_end(); ++e)
            {
               // if the arc is in the matching (i.e., reversed), f is not critical
               if (EM(e.from_node(), e.to_node()))
               {
                  isCritical = false;
                  break;
               }
            }
         }
         if ( isCritical )
            critical += M.face(*f);
      }
   }
   return critical;
}





/**@brief Compute the size of an EdgeMap
 * @param EM the EdgeMap
 * @returns the number of the arcs marked with @c true
 */
template <class EdgeMap>
int EdgeMapSize(const EdgeMap& EM)
{
   int size = 0;
   for (typename Entire<EdgeMap>::const_iterator e = entire(EM); !e.at_end(); ++e)
      if (*e) ++size;
   return size;
}



/**@class CompareByProperty
 * @brief Compares two objects by their value given by a property
 */
template <class Object, class Property>
class CompareByProperty
{
public:
   CompareByProperty(const Property& P) : P_(P)
   {}

   bool operator() (const Object &p, const Object &q) const
   {
      if ( P_[p] < P_[q] )
         return true;
      return false;
   }

private:
   const Property& P_;
};





/**@brief Compute lexicographic ordering of the edges
 * @param M            subset of the arcs
 * @param order        output: order of arcs
 * @param bottomLevel  lowest level
 * @param topLevel     hightest level
 *
 * - We loop through each level, beginning at the top
 * - We sort the faces in a level lexicographically.
 * - For each face @a F in the lexicographic order,
 *   we sort its facets lexicographically and insert
 *   the corresponding arc numbers into the order.
 *
 * @a order is a std::vector, because we want to change its size dynamically.
 * Might be replaced by @c Array and precomputation of the size.
 */
template <class vect>
void orderEdgesLex(const graph::HasseDiagram& M, vect& order, int bottomLevel, int topLevel)
{
   int d = M.dim() - 1;      // do not count empty face

   order.clear();

   // create edge-number map
   Map<std::pair<int,int>, int> E;   // map to store numbers for edges

   int m = 0;
   for (int k = 0; k < d; ++k)
      for (Entire<graph::HasseDiagram::nodes_of_dim_set>::const_iterator f = entire(M.nodes_of_dim(k)); !f.at_end(); ++f)
         for (HasseDiagramOutConstIterator e = M.out_edges(*f).begin(); !e.at_end(); ++e)
            E[std::make_pair(e.from_node(), e.to_node())] = m++;

   // loop over levels, starting from the top most
   for (int k = topLevel; k >= bottomLevel; --k)
   {
      // collect all nodes of dimension k
      std::vector<Set<int> > FS;    // vertex sets of faces
      std::vector<int> FN;          // face numbers
      std::vector<int> index;       // indices to sort
      const int n = M.node_range_of_dim(k).size();
      FS.reserve(n); FN.reserve(n); index.reserve(n);
      int i = 0;
      for (Entire<graph::HasseDiagram::nodes_of_dim_set>::const_iterator f = entire(M.nodes_of_dim(k)); !f.at_end(); ++f)
      {
         const Set<int> F = M.face(*f);  // get vertex set corresponding to face
         FS.push_back(F);          // store set and
         FN.push_back(*f);         // face number
         index.push_back(i++);
      }

      // sort
      CompareByProperty<int,std::vector<Set<int> > > cmp(FS);
      std::sort(index.begin(), index.end(), cmp);

      // pass through faces of dim k in lexicographic order
      std::vector<int>::const_iterator fit, fend = index.end();
      for (fit = index.begin(); fit != fend; ++fit)
      {
         const Set<int> F = FS[*fit];   // get current face

         // collect neighbors of F
         std::vector<Set<int> > N;
         std::vector<int> EN;
         std::vector<int> index2;
         const int n = M.in_edges(FN[*fit]).size();
         N.reserve(n); EN.reserve(n); index2.reserve(n);
         int l = 0;
         for (HasseDiagramInConstIterator e = M.in_edges(FN[*fit]).begin(); !e.at_end(); ++e)
         {
            const Set<int> S = M.face(e.from_node());   // get vertex set corr. to neighbor
            N.push_back(S);                       // store it
            EN.push_back(E[std::make_pair(e.from_node(), e.to_node())]);  // store edge number
            index2.push_back(l++);
         }
         
         // sort
         CompareByProperty<int,std::vector<Set<int> > > Cmp(N);
         std::sort(index2.begin(), index2.end(), Cmp);

         /// pass through neighbors in lexicographic order
         std::vector<int>::const_iterator it, iend = index2.end();
         for (it = index2.begin(); it != iend; ++it)
            order.push_back(EN[*it]);
      }
   }
}



//-------------------------------------------------------------------------------------------------------------
//------------------------------------- Greedy Algorithm ------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------



/**@brief Compute a Morse matching by a simple greedy heuristic
 * @param M         Hasse diagram
 * @param varLevel  level of arcs
 * @param varOrder  order of arcs
 *
 * @note @a varOrder can contain a subset of the arc indices, but
 * @a varLevel stores the level of @b all arcs.
 */
template <class LevelMap, class Iterator>
int greedyHeuristic(graph::HasseDiagram& M, HasseEdgeMap& EM, const LevelMap& varLevel, Iterator orderIt, Iterator orderEnd)
{
   typedef Graph<Directed>::out_edge_list::const_iterator HasseDiagramOutConstIterator;

   int d = M.dim() - 1;
   int n = M.nodes() - 2;
   int size = 0;
   int m = varLevel.size();

   Array<bool> matched(n+1);
   Array<int> marked(n+1);
   Array<HasseDiagramOutConstIterator> V(m);

   // init solution to the empty set and collect edge iterators
   int i = 0;
   for (int k = 0; k < d; ++k)
   {
      for (Entire<graph::HasseDiagram::nodes_of_dim_set>::iterator f = entire(M.nodes_of_dim(k)); !f.at_end(); ++f)
      {
         for (HasseDiagramOutConstIterator e = M.out_edges(*f).begin(); !e.at_end(); ++e)
         {
            EM(e.from_node(), e.to_node()) = false;
            V[i] = e;
            ++i;
         }
      }
   }
   assert(i <= m);

   // init to no matched or marked faces
   for (int i = 0; i <= n; ++i)
   {
      matched[i] = false;
      marked[i] = 0;
   }

   int base = 1;
   for (; orderIt != orderEnd; ++orderIt)
   {
      int var = *orderIt;
      HasseDiagramOutConstIterator e = V[var];
      int source = e.from_node();
      int target = e.to_node();

      if ((! matched[source]) && (! matched[target]) )
      {
         // tentatively include var in solution and check ...
         EM(source, target) = true;

         assert( M.dim_of_node(source) < M.dim_of_node(target) );

         if ( checkAcyclicDFS(M, EM, marked, source, true, base) )
         {
            ++size;
            matched[source] = true;
            matched[target] = true;
         }
         else
            EM(source, target) = false;
         // increase base
         base += 2;
      }
   }
   return size;
}







//-------------------------------------------------------------------------------------------------------------
//------------------------------------- Find alternating paths ------------------------------------------------
//-------------------------------------------------------------------------------------------------------------



/**@brief  Depth first search to find alternating paths
 * @param M         Morse matching in Hasse diagram
 * @param marked    marker for dfs-search (node has been visited)
 * @param p         stores the DFS-tree edges
 * @param v         start node
 * @param lower     whether we are on the lower part
 *
 * This is a modified DFS, see processAlternatingPaths for a description.
 */
template <class EdgeMap>
void findAlternatingPathDFS(const graph::HasseDiagram& M, const EdgeMap& EM, Array<int>& marked, Array<int>& p, int v, bool lower)
{
   marked[v] = 1;
   if (lower)  // if we are one the lower side of a level
   {
      for (HasseDiagramOutConstIterator e = M.out_edges(v).begin(); !e.at_end(); ++e)
      {
         if (EM(e.from_node(), e.to_node()))
         {
            int u = e.to_node();
            if (marked[u] == 0)
            {
               p[u] = v;
               findAlternatingPathDFS(M, EM, marked, p, u, false);
            }
            else
               ++(marked[u]);
         }
      }
   }
   else
   {
      for (HasseDiagramInConstIterator e = M.in_edges(v).begin(); ! e.at_end(); ++e)
      {
         if (! EM(e.from_node(), e.to_node()) )
         {
            int u = e.from_node();
            if (marked[u] == 0)
            {
               p[u] = v;
               findAlternatingPathDFS(M, EM, marked, p, u, true);
            }
            else
               ++(marked[u]);
         }
      }
   }
}




/**@brief Exchange matching arcs on alternating paths
 * @param M     Morse matching in Hasse diagram
 * @param p     stores the DFS-tree edges
 * @param v     start node
 * @param v     end node
 * @param size  size of the matching
 */
template <class EdgeMap>
void exchangePath(const graph::HasseDiagram& M, EdgeMap& EM, const Array<int>& p, int u, int v, int& size)
{
   int w = v;   // start node
   do
   {
      int ww = p[w];
      if ( M.edge_exists(w, ww) )
      {
         const bool r = EM(w,ww);
         EM(w,ww) = !r;
         if ( r )
            --size;
         else
            ++size;
      }
      else
      {
         const bool r = EM(ww,w);
         EM(ww,w) = !r;
         if ( r )
            --size;
         else
            ++size;
      }
      w = ww;
   }
   while (w != u);
}







/**@brief Improve a Morse matching by canceling critical faces along alternating paths.
 * @param M            Morse matching in Hasse diagram
 * @param size         size of the computed solution
 * @param bottomLevel  lowest level to consider
 * @param topLevel     highest level to consider
 *
 * An alternating path w.r.t. a given Morse matching @a M is a path in the modified
 * Hasse diagram @a H(M) starting at a critical face and ending at a critical face;
 * it is called alternating because normal arcs and flipped arcs alternate on this
 * path. Forman observed that if there exists a @a unique alternating path then
 * one can flip the direction of every arc in the path to again obtain a Morse
 * matching. The two critical faces at the end of the path are not critical w.r.t.
 * the new Morse matching.
 *
 * Alternating paths can be found by a modified DFS as follows. We work on the
 * acyclic graph @a H(M) and start at a critical face @a u and then recurse. Initially
 * all faces have been marked 0. Let @a v be the current face and @a a = (@a v,@a w)
 * be an out-arc of @a v. If @a w has not been visited (its mark is 0), then we mark
 * @a w with 1 and set the DFS tree arc to be @a a. If @a w has been visited (marked
 * with a positive number) then we increase its mark by one.
 *
 * After running this algorithm we can check each critical face @a v (not equal
 * to @a u). If its mark is 1, we check whether each face on the path from @a v to
 * @a u is marked with 1. If this is the case, then we found a unique path from @a u
 * to @a v. This can be seen as follows:
 *
 * If there exists a unique path, then every face @a w on the path to @a v is visited
 * exactly once (so it is marked with 1).
 *
 * Conversely, assume that the path is not unique, i.e. there exist at least two
 * distinct paths from @a u to @a v (if there exists no path, then @a v is not marked).
 * DFS follows each path until it reaches an already visited face @a w; then the mark
 * of @a w is increased by one. Hence, at least one face on every path from @a u to
 * @a v has mark at least 2 (twice visisted).
 *
 * The algorithm is then simple to describe. We first compute all critical faces
 * w.r.t. the current solution. We loop over all critical faces @a u and run the
 * modified DFS. If there exists a critical face such that each face on the path to
 * @a u is marked with 1, we flip all arcs and set the two endpoints as non-critical.
 *
 * Note that the result may depend on the order of the paths flipped.
 */
template <class EdgeMap>
void processAlternatingPaths(graph::HasseDiagram& M, EdgeMap& EM, int& size, int bottomLevel, int topLevel)
{
#if POLYMAKE_DEBUG
   const bool debug_print = perl::get_debug_level() > 1;
#endif
   int n = M.nodes() - 2;
   int cnt = 0; // number of alternating paths

   // compute critical faces
   Bitset critical = collectCriticalFaces(M, EM);

   // find alternating paths
   Array<int> marked(n+1);
   Array<int> dfsTree(n+1);
   for (int k = bottomLevel; k < topLevel; ++k)
   {
      // check only nodes at the upper level
      for (Entire<graph::HasseDiagram::nodes_of_dim_set>::const_iterator f = entire(M.nodes_of_dim(k+1)); !f.at_end(); ++f)
      {
         int u = *f;
         if ( critical.contains(u) )
         {
            for (int i = 0; i <= n; ++i)
            {
               dfsTree[i] = -1;
               marked[i] = 0;
            }

            findAlternatingPathDFS(M, EM, marked, dfsTree, u, false);

            // check whether we found a path: lower part
            for (Entire<graph::HasseDiagram::nodes_of_dim_set>::const_iterator f2 = entire(M.nodes_of_dim(k)); !f2.at_end(); ++f2)
            {
               int v = *f2;

               //if ( (v != u) && critical.contains(v) && (marked[v] == 1) )
               if ( critical.contains(v) && (marked[v] == 1) )
               {
                  // check whether path contains only nodes visited once
                  int w = v;
                  do
                  {
                     w = dfsTree[w];
                     assert( w >= 0 );
                     if ( marked[w] != 1 )
                        break;
                  }
                  while (w != u);

                  if (w == u)
                  {
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
template <class Graph, class EdgeMap, class PMap>
void findMaximumForest(const Graph& G, const EdgeMap& EM, PMap& p, Array<int>& visited)
{
   typedef typename Graph::out_edge_list::const_iterator GraphOutConstIterator;

   // initialize visited:
   for (int i = 0; i < G.nodes(); ++i)
      visited[i] = 0;

   // search for start vertex and then start BFS iteration
   for (int start = 0; start < G.nodes(); ++start)
   {
      if ( visited[start] == 0 )   // if vertex has not be seen so far
      {
         visited[start] = 2;   // mark vertex as root
         std::deque<int> Q;

         // initialize edges from start vertex
         for (GraphOutConstIterator e = G.out_edges(start).begin(); !e.at_end(); ++e)
         {
            const int w = e.to_node();
            if ( visited[w] == 0 )
            {
               p[w] = EM(e.from_node(), e.to_node());
               Q.push_back(w);
            }
         }

         // start BFS iterations
         while (! Q.empty() )
         {
            const int v = Q.front();
            Q.pop_front();
            visited[v] = 1;

            for (GraphOutConstIterator e = G.out_edges(v).begin(); !e.at_end(); ++e)
            {
               const int w = e.to_node();
               if ( visited[w] == 0 )
               {
                  p[w] = EM(e.from_node(), e.to_node());
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
template <class Graph, class EdgeMap, class PMap, class MarkedMap>
void findMaximumForestMarked(const Graph& G, const EdgeMap& EM, MarkedMap& marked, PMap& p, Array<int>& visited)
{
   typedef typename Graph::out_edge_list::const_iterator GraphOutConstIterator;

   // initialize visited:
   for (int i = 0; i < G.nodes(); ++i)
      visited[i] = 0;

   // first check for marked nodes
   for (int start = 0; start < G.nodes(); ++start)
   {
      if ( visited[start] == 0 && marked[start] )   // if loop is not visited
      {
         visited[start] = 2;
         std::deque<int> Q;

         // initialize edges from start vertex
         for (GraphOutConstIterator e = G.out_edges(start).begin(); !e.at_end(); ++e)
         {
            const int w = e.to_node();
            if ( visited[w] == 0 )
            {
               p[w] = EM(e.from_node(), e.to_node());
               Q.push_back(w);
            }
         }

         // start BFS iterations
         while (! Q.empty() )
         {
            const int v = Q.front();
            Q.pop_front();
            visited[v] = 1;

            for (GraphOutConstIterator e = G.out_edges(v).begin(); !e.at_end(); ++e)
            {
               const int w = e.to_node();
               if ( visited[w] == 0 )
               {
                  p[w] = EM(e.from_node(), e.to_node());
                  Q.push_back(w);
               }
            }
         }
      }
   }

   // search for start vertex and then start BFS iteration
   for (int start = 0; start < G.nodes(); ++start)
   {
      if ( visited[start] == 0 )   // if vertex has not be seen so far
      {
         visited[start] = 2;   // mark vertex as root
         std::deque<int> Q;

         // initialize edges from start vertex
         for (GraphOutConstIterator e = G.out_edges(start).begin(); !e.at_end(); ++e)
         {
            const int w = e.to_node();
            if ( visited[w] == 0 )
            {
               p[w] = EM(e.from_node(), e.to_node());
               Q.push_back(w);
            }
         }

         // start BFS iterations
         while (! Q.empty() )
         {
            const int v = Q.front();
            Q.pop_front();
            visited[v] = 1;

            for (GraphOutConstIterator e = G.out_edges(v).begin(); !e.at_end(); ++e)
            {
               const int w = e.to_node();
               if ( visited[w] == 0 )
               {
                  p[w] = EM(e.from_node(), e.to_node());
                  Q.push_back(w);
               }
            }
         }
      }
   }
}





//-------------------------------------------------------------------------------------------------------------
//------------------------------------- Complete Solution -----------------------------------------------------
//-------------------------------------------------------------------------------------------------------------

template <class Graph, class EdgeMap>
void make_edges_in_Gamma(const graph::HasseDiagram& M, const HasseEdgeMap& EM, const Map<int,int>& FTON, Graph& Gamma, EdgeMap& edge_map_Gamma)
{
   for (Entire<graph::HasseDiagram::nodes_of_dim_set>::const_iterator f = entire(M.nodes_of_dim(1)); !f.at_end(); ++f) { // iterate over all 1-faces of the complex
      bool is_unmatched_edge (true);
      for (HasseDiagramOutConstIterator e = M.out_edges(*f).begin(); !e.at_end() && is_unmatched_edge; ++e) { 
         assert(M.dim_of_node(e.to_node()) == 2); // Assuming that out_edges go to the 2-skeleton
         if (EM(e.from_node(), e.to_node())) 
            is_unmatched_edge = false; // if a 1-face is matched to a 2-face, it doesn't count
      }
      if (is_unmatched_edge) {
         const Set<int> e = M.in_adjacent_nodes(*f);
         assert(e.size()==2);
         const int v1 = e.front(), v2 = e.back();

         Gamma.edge(FTON[v1], FTON[v2]);
         edge_map_Gamma(FTON[v1], FTON[v2]) = *f;
      }
   }
}

template <class EdgeMap>
void remove_matching_from_1_skeleton(const graph::HasseDiagram& M, EdgeMap& EM)
{
   for (Entire<sequence>::const_iterator n = entire(M.node_range_of_dim(0)); !n.at_end(); ++n)
      for (HasseDiagramOutConstIterator e = M.out_edges(*n).begin(); !e.at_end(); ++e)
         EM(e.from_node(), e.to_node()) = false;
}


/**@brief Complete the Morse matching to the bottom level by a maximum forest computation
 * @param M   Morse matching
 *
 * The current Morse matching in @a M is extended to the bottom level as follows:
 *
 * -# Compute the currently critical faces.
 * -# Generate the graph of the complex with non-critical edges removed.
 * -# ie, Generate the graph Gamma obtained from the graph of the complex by removing all 
 *    1-faces matched with 2-faces
 * -# Compute a maximum forest on the graph.
 * -# The forest is given by edges oriented towards a root.
 * -# The matching is extended by the vertices matched with the corresponding
 *    edges leading in the direction of the root.
 *
 * Currently we assume that the complex is connected. It follows that the graph
 * is connected as well (see Joswig & Pfetsch [2005]).
 */
template <class _EdgeMap>
void completeToBottomLevel(graph::HasseDiagram& M, _EdgeMap& EM)
{
   // find critical faces of best_solution_
   Bitset critical = collectCriticalFaces(M, EM);
#if POLYMAKE_DEBUG
   const bool debug_print = perl::get_debug_level() > 1;

   if (debug_print) {
      cout << "critical faces: " << critical << endl;
      for (Entire<Bitset>::const_iterator c = entire(critical); !c.at_end(); ++c)
         cout << M.face(*c) << " ";
      cout << endl;
   }
#endif

   // build helper graph
   Graph<Undirected> Gamma;
   NodeMap<Undirected, int> node_map_Gamma(Gamma);
   EdgeMap<Undirected, int> edge_map_Gamma(Gamma);

   // to translate between face indices and nodes of the graph:
   Map<int, int> FTON;

   // create nodes of the graph ( = vertices (0-faces) of complex )
   for (Entire<graph::HasseDiagram::nodes_of_dim_set>::const_iterator f = entire(M.nodes_of_dim(0)); !f.at_end(); ++f)
   {
      const int v = Gamma.add_node();  // add 1-face
      node_map_Gamma[v] = *f;
      FTON[*f] = v;
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
   int numNodes = Gamma.nodes();
   Array<int> visited(numNodes);
   Array<int> p(numNodes);

   findMaximumForest(Gamma, edge_map_Gamma, p, visited);

#if POLYMAKE_DEBUG
   if (debug_print) {
      cout << "visited: " << visited << endl
           << "p: " << p << endl
           << "faces of p: ";
      for (int i=0; i<numNodes; ++i)
         cout << M.face(p[i]) << " ";
      cout << endl;
   }
#endif

   remove_matching_from_1_skeleton(M, EM);

   // now add corresponding arcs to the matching
   for (int i = 0; i < numNodes; ++i) {
      assert( visited[i] != 0);
      // if vertex is in the forest, but not the root
      if (visited[i] == 1) {
         const int fnum = p[i];       // index of 1-face in complex
         const int vnum = node_map_Gamma[i];  // index of 0-face in complex
         EM(vnum, fnum) = true;
      }
   }
}






/**@brief Complete the Morse matching to the top level by a maximum forest computation
 * @param M   Morse matching
 *
 * This is only possible if M arises from a pseudo-manifold.
 *
 * The current Morse matching in @a M is extended to the top level as in
 * @c completeToBottomLevel(), but with the following exceptions:
 *
 * - We work on the dual graph of the complex.
 * - The boundary faces produce a loop in the dual graph.
 * - When computing a spanning forest we first try to take vertices with a loop
 *   as the root, since this produces a component without critical face.
 * - When completing the matching, the arcs corresponding to loops are added.
 *
 */
template <class _EdgeMap>
void completeToTopLevel(const graph::HasseDiagram& M, _EdgeMap& EM)
{
   typedef Entire<Graph<Directed>::out_edge_list>::const_iterator HasseDiagramOutConstIterator;
   int d = M.dim() - 1;

   // find critical faces of best_solution_
   Bitset critical = collectCriticalFaces(M, EM);
   
   // build helper graph
   Graph<Directed> G;
   NodeMap<Directed, int> node_map_G(G);
   EdgeMap<Directed, int> edge_map_G(G);

   // to translate between face indices and nodes of the graph:
   Map<int, int> FTON;

   // to store loops
   std::vector<bool> loop;

   // create nodes of the graph ( = vertices (d-faces) of complex )
   for (Entire<graph::HasseDiagram::nodes_of_dim_set>::const_iterator f = entire(M.nodes_of_dim(d)); !f.at_end(); ++f)
   {
      int v = G.add_node();  // add d-face
      node_map_G[v] = *f;
      FTON[*f] = v;
      loop.push_back(false);   // assume that node numbers a consecutive
   }

   // create edges (arising from 1-faces in the complex)
   for (Entire<graph::HasseDiagram::nodes_of_dim_set>::const_iterator f = entire(M.nodes_of_dim(d-1)); !f.at_end(); ++f)
   {
      if ( critical.contains(*f) )
      {
         // find the two nodes incident to the edge
         int v1 = -1;
         int v2 = -1;
         for (HasseDiagramOutConstIterator e = M.out_edges(*f).begin(); !e.at_end(); ++e)
         {
            if (v1 == -1)
               v1 = e.to_node();
            else
            {
               assert( v2 == -1);
               v2 = e.to_node();
            }
         }
         assert(v1 != -1);

         // either produce an edge or a loop:
         if (v2 != -1) {
            G.edge(FTON[v1], FTON[v2]);
            edge_map_G(FTON[v1], FTON[v2]) = *f;
         } else
            loop[FTON[v1]] = true;   // store that the d-face corr. to a loop
      }
   }

   // prepare computation of spanning forest
   const int numNodes = G.nodes();
   Array<int> visited(numNodes);
   Array<int> p(numNodes);

   findMaximumForestMarked(G, edge_map_G, loop, p, visited);

   // now add corresponding arcs to the matching
   for (int i = 0; i < numNodes; ++i)
   {
      assert( visited[i] != 0);
      // if vertex is in the forest or the root and a loop
      if ( visited[i] == 1 || (visited[i] == 2 && loop[i]) )
      {
         int rnum = p[i];       // index of (d-1)-face in complex
         int fnum = node_map_G[i];  // index of d-face in complex
         EM(rnum, fnum) = true;
      }
   }
}

template <class EdgeMap>
void print_reversed_edges(const graph::HasseDiagram& M, const EdgeMap& EM)
{
   cout << "critical Morse edges:\n";
   for (Entire<Edges<Graph<Directed> > >::const_iterator e = entire(edges(M.graph())); !e.at_end(); ++e)
      if (EM(e.from_node(), e.to_node()))
         cout << "(" << M.face(e.from_node()) << "," << M.face(e.to_node()) << ")";
   cout << endl;
}

} }

#endif // POLYMAKE_TOPAZ_MORSE_MATCHING_TOOLS_H
    
// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
