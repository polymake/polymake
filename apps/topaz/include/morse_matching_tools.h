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

   @author Marc Pfetsch
   @brief  tools for computations with Morse matchings
   @todo Check whether overflow can occur in @a base (see checkAcyclicDFS() ).
*/

#pragma once

#include "polymake/client.h"
#include "polymake/graph/connected.h"
#include "polymake/Array.h"
#include "polymake/Map.h"
#include "polymake/PowerSet.h"
#include "polymake/graph/ShrinkingLattice.h"
#include "polymake/graph/Decoration.h"
#include "polymake/vector"
#include <cassert>
#include <deque>

namespace polymake { namespace topaz {
namespace morse_matching_tools {

using MorseEdgeMap = EdgeMap<Directed, Int>;

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
bool checkAcyclicDFS(const graph::ShrinkingLattice<graph::lattice::BasicDecoration>& M, const MorseEdgeMap& EM, Array<Int>& marked, Int v, bool lower, Int base);


/**@brief Find the critical faces w.r.t the Morse matching in @a M
 * @param M         Morse matching in Hasse diagram
 * @returns bitset indicating the critical faces (nodes)
 *
 * On output an element (node) is set to true in the bitset if and only if the face is critical.
 */
Bitset collectCriticalFaces(const graph::ShrinkingLattice<graph::lattice::BasicDecoration>& M, const MorseEdgeMap& EM);


/**@brief Compute the size of an EdgeMap
 * @param EM the EdgeMap
 * @returns the number of the arcs marked with @c true
 */
inline
Int count_marked_edges(const MorseEdgeMap& EM)
{
   Int size = 0;
   for (auto e = entire(EM); !e.at_end(); ++e)
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
template <typename LevelMap, typename Iterator>
Int greedyHeuristic(graph::ShrinkingLattice<graph::lattice::BasicDecoration>& M, EdgeMap<Directed, Int>& EM, const LevelMap& varLevel, Iterator orderIt, Iterator orderEnd)
{
   using HasseDiagramOutConstIterator = Graph<Directed>::out_edge_list::const_iterator;

   const Int d = M.rank()-2;
   const Int n = M.nodes()-2;
   Int size = 0;
   Int m = varLevel.size();

   std::vector<bool> matched(n+1);
   Array<Int> marked(n+1);
   Array<HasseDiagramOutConstIterator> V(m);

   // init solution to the empty set and collect edge iterators
   for (Int i = 0, k = 0; k < d; ++k) {
      for (const auto f : M.nodes_of_rank(k+1)) {
         for (auto e = M.out_edges(f).begin(); !e.at_end(); ++e) {
            EM[*e] = false;
            V[i] = e;
            ++i;
            assert(i <= m);
         }
      }
   }

   // init to no matched or marked faces
   for (Int i = 0; i <= n; ++i)
   {
      matched[i] = false;
      marked[i] = 0;
   }

   Int base = 1;
   for (; orderIt != orderEnd; ++orderIt)
   {
      Int var = *orderIt;
      HasseDiagramOutConstIterator e = V[var];
      Int source = e.from_node();
      Int target = e.to_node();

      if ((! matched[source]) && (! matched[target]) )
      {
         // tentatively include var in solution and check ...
         EM(source, target) = true;

         assert( M.rank(source) < M.rank(target) );

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
void findAlternatingPathDFS(const graph::ShrinkingLattice<graph::lattice::BasicDecoration>& M, const MorseEdgeMap& EM, Array<Int>& marked, Array<Int>& p, Int v, bool lower);


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

void processAlternatingPaths(graph::ShrinkingLattice<graph::lattice::BasicDecoration>& M, MorseEdgeMap& EM, Int& size, Int bottomLevel, Int topLevel);

//-------------------------------------------------------------------------------------------------------------
//------------------------------------- Complete Solution -----------------------------------------------------
//-------------------------------------------------------------------------------------------------------------

void make_edges_in_Gamma(const graph::ShrinkingLattice<graph::lattice::BasicDecoration>& M, const MorseEdgeMap& EM,
                         const Map<Int, Int>& FTON, Graph<Undirected>& Gamma, EdgeMap<Undirected, Int>& edge_map_Gamma);

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
void completeToBottomLevel(graph::ShrinkingLattice<graph::lattice::BasicDecoration>& M, MorseEdgeMap& EM);


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
void completeToTopLevel(const graph::ShrinkingLattice<graph::lattice::BasicDecoration>& M, MorseEdgeMap& EM);

} } }

    
// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
