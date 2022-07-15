/* Copyright (c) 1997-2022
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
#include "polymake/graph/Lattice.h"
#include "polymake/graph/ShrinkingLattice.h"
#include "polymake/graph/Decoration.h"
#include "polymake/Array.h"
#include "polymake/topaz/morse_matching_tools.h"
#include <cassert>


namespace polymake { namespace topaz {

namespace {

using namespace morse_matching_tools;

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
void orderEdgesLex(const graph::ShrinkingLattice<graph::lattice::BasicDecoration>& M, std::vector<Int>& order, Int bottomLevel, Int topLevel)
{
   const Int d = M.rank()-2;      // do not count empty face

   order.clear();

   // create edge-number map
   EdgeMap<Directed, Int> E(M.graph());

   Int m = 0;
   for (Int k = 0; k < d; ++k)
      for (const auto f : M.nodes_of_rank(k+1))
         for (auto e = M.out_edges(f).begin(); !e.at_end(); ++e)
            E[*e] = m++;

   // loop over levels, starting from the top most
   for (Int k = topLevel; k >= bottomLevel; --k) {
      // collect all nodes of dimension k
      std::vector<Set<Int> > FS;    // vertex sets of faces
      std::vector<Int> FN;          // face numbers
      std::vector<Int> index;       // indices to sort
      const Int n_nodes = M.nodes_of_rank(k+1).size();
      FS.reserve(n_nodes); FN.reserve(n_nodes); index.reserve(n_nodes);
      Int i = 0;
      for (const auto f : M.nodes_of_rank(k+1)) {
         const Set<Int>& F = M.face(f);  // get vertex set corresponding to face
         FS.push_back(F);          // store set and
         FN.push_back(f);         // face number
         index.push_back(i++);
      }

      // sort
      CompareByProperty<Int, std::vector<Set<Int>>> cmp(FS);
      std::sort(index.begin(), index.end(), cmp);

      // pass through faces of dim k in lexicographic order
      for (const Int fi : index)
      {
         const Set<Int> F = FS[fi];   // get current face

         // collect neighbors of F
         std::vector<Set<Int>> N;
         std::vector<Int> EN;
         std::vector<Int> index2;
         const Int n_edges = M.in_edges(FN[fi]).size();
         N.reserve(n_edges); EN.reserve(n_edges); index2.reserve(n_edges);
         Int l = 0;
         for (auto e = M.in_edges(FN[fi]).begin(); !e.at_end(); ++e)
         {
            const Set<Int> S = M.face(e.from_node());   // get vertex set corr. to neighbor
            N.push_back(S);                       // store it
            EN.push_back(E[*e]);  // store edge number
            index2.push_back(l++);
         }

         // sort
         CompareByProperty<Int, std::vector<Set<Int>>> Cmp(N);
         std::sort(index2.begin(), index2.end(), Cmp);

         /// pass through neighbors in lexicographic order
         for (const Int fi2 : index2)
            order.push_back(EN[fi2]);
      }
   }
}

#ifndef NDEBUG
#if 0
// these two are unused now

/**@brief Check whether a given solution is acyclic
 * @param M         collection of arcs in Hasse diagram
 * @param marked    marker for DFS-search (node has been visited)
 * @param base      base value for marker (see @c checkAcyclicDFS())
 * @returns @c true if the solution is acyclic
 *
 * We assume that @c marked has been initialized some time ago.
 * @warning @a base is changed!
 */
bool checkAcyclic(const graph::ShrinkingLattice<graph::lattice::BasicDecoration>& M, const MorseEdgeMap& EM,  Array<Int>& marked, Int& base)
{
   const Int d = M.rank()-2;      // do not count empty face and top face

   // check each level in turn
   for (Int k = 0; k < d; ++k) {
      // increase base
      base += 2;

      // check each face of dim k
      for (const auto v : M.nodes_of_rank(k+1)) {
         assert( v > 0 && v <= (M.nodes()-2) );

         // for each unmarked face
         if (marked[v] < base) {
            const bool acyclic = checkAcyclicDFS(M, EM, marked, v, true, base);
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
bool checkAcyclic(const graph::ShrinkingLattice<graph::lattice::BasicDecoration>& M, const MorseEdgeMap& EM, Int k, Array<Int>& marked, Int base)
{
   // check each face of dim k
   for (const auto v : M.nodes_of_rank(k+1)) {
      assert( v > 0 && v <= (M.nodes()-2) );

      // for each unmarked face
      if (marked[v] < base) {
         const bool acyclic = checkAcyclicDFS(M, EM, marked, v, true, base);
         if (! acyclic)
            return false;
      }
   }
   return true;
}
#endif

/**@brief Check whether a given solution is acyclic
 * @param M  collection of arcs in Hasse diagram
 * @returns @c true if the solution is acyclic
 */
bool checkAcyclic(const graph::ShrinkingLattice<graph::lattice::BasicDecoration>& M, const MorseEdgeMap& EM)
{
   const Int d = M.rank()-2;      // do not count empty face
   const Int n = M.nodes()-2;    // and top face

   Array<Int> marked(n+1);
   // initialize markers
   for (Int i = 0; i <= n; ++i)
      marked[i] = 0;

   // for each level in turn
   Int base = -1;
   for (Int k = 0; k < d; ++k) {
      // increase base
      base += 2;

      // check each face of dim k
      for (const auto v : M.nodes_of_rank(k+1)) {
         assert( (v > 0) && (v <= n) );

         // for each unmarked face
         if (marked[v] < base) {
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
bool checkMatching(const graph::ShrinkingLattice<graph::lattice::BasicDecoration>& M, const MorseEdgeMap& EM)
{
#if POLYMAKE_DEBUG
   const bool debug_print = get_debug_level() > 1;
#endif

   const Int d = M.rank()-2;

   // for each level in turn
   for (Int k = 0; k <= d; ++k) {
      // check each face of dim k
      for (const auto f : M.nodes_of_rank(k+1)) {
         Int inc = 0;  // count incident arcs
         if (k > 0) {
            for (auto e = M.in_edges(f).begin(); !e.at_end(); ++e) {
               // if arc is in collection, i.e. reverse we have an incident arc
               if (EM[*e])
                  ++inc;
               // if a node has more than two incident arcs the matching property is violated
               if (inc >= 2) {
#if POLYMAKE_DEBUG
                  if (debug_print) cout << "Matching propery violated for node " << f << "=" << M.face(f) << endl;
#endif
                  return false;
               }
            }
         }
         if (k < d) {
            for (auto e = M.out_edges(f).begin(); !e.at_end(); ++e) {
               // if arc is in collection, i.e. reverse we have an incident arc
               if (EM[*e])
                  ++inc;
               // if a node has more than two incident arcs the matching property is violated
               if (inc >= 2) {
#if POLYMAKE_DEBUG
                  if (debug_print) cout << "Matching propery violated for node " << f << "=" << M.face(f) << endl;
#endif
                  return false;
               }
            }
         }
      }
   }
   return true;
}
#endif
#if POLYMAKE_DEBUG
void print_reversed_edges(const graph::ShrinkingLattice<graph::lattice::BasicDecoration>& M, const MorseEdgeMap& EM)
{
   cout << "critical Morse edges:\n";
   for (auto e = entire(edges(M.graph())); !e.at_end(); ++e)
      if (EM[*e])
         cout << "(" << M.face(e.from_node()) << "," << M.face(e.to_node()) << ")";
   cout << endl;
}
#endif
}

// Compute a Morse matching. Two heuristics are implemented: 
//
// (1) A simple greedy algorithm:
//    The arcs are visited in lexicographical order, i.e.
//    we proceed by levels from top to bottom,
//    visit the faces in each dimension in lexicographical order,
//    and visited the faces covered by these faces in lexicographical order.
//    This heuristic is used by default and with heuristic => 1.
//
// (2) A Morse matching can be improved by canceling critical cells
//    along unique alternating paths, see function 
//    processAlternatingPaths() in file morse_matching_tools.cc .
//    This idea is due to Robin Forman: 
//       Morse Theory for Cell-Complexes,
//       Advances in Math., 134 (1998), pp. 90-145.
//    This heuristic is used by default and with heuristic => 2. 
//
// default setting is to use both, i.e., to run the greedy algorithm 
// and then improve the result by the canceling algorithm. 
//
// Morse matchings for the bottom level can be found optimally by
// spanning tree techniques. This can be enabled by the option
// levels => 1.  If the complex is a pseudo-manifold the same can be
// done for the top level (option levels => 2). By specifying option
// levels => 0, both levels can be computed by spanning trees.
// For 2-dim pseudo-manifolds this computes an optimal Morse matching.
                  
      

MorseEdgeMap morse_matching(BigObject p, OptionSet options)
{
   // determine which heuristic to run
   bool runGreedy = true;  // default is to turn both heuristics on
   bool runCancel = true;
   const Int heuristic = options["heuristic"];
   if (heuristic == 1) runCancel = false;
   if (heuristic == 2) runGreedy = false;

#if POLYMAKE_DEBUG
   const bool debug_print = get_debug_level() > 1;
   if (debug_print)
      cout << "heuristic: " << heuristic 
           << ", runGreedy=" << runGreedy 
           << ", runCancel=" << runCancel << endl;
#endif

   graph::Lattice<graph::lattice::BasicDecoration> M_obj = p.give("HASSE_DIAGRAM");
   graph::ShrinkingLattice<graph::lattice::BasicDecoration> M(M_obj); // not const, will be modified
   const Int d = M.rank()-2;
   Int size = 0;

#if POLYMAKE_DEBUG
   if (debug_print) cout << "d: " << d << endl;
#endif

   // find lowest and highest levels
   Int bottomLevel = 0;
   Int topLevel = d;
   const Int levels = options["levels"];
   if (levels == 1)
      bottomLevel = 1;
   if (levels == 2)
      topLevel = d-1;
   if (levels == 0)  {
      bottomLevel = 1;
      topLevel = d-1;
   }

#if POLYMAKE_DEBUG
   if (debug_print)
      cout << "levels: " << levels
           << ", bottomLevel=" << bottomLevel
           << ", topLevel=" << topLevel << endl;
#endif

   MorseEdgeMap EM(M.graph());

#if POLYMAKE_DEBUG
   if (debug_print) cout << "Initialized EdgeMap" << endl;
#endif

   // check whether complex is a pseudo-manifold
   if (topLevel < d) {
      const bool is_pmf = p.give("PSEUDO_MANIFOLD");
      if (!is_pmf) 
         throw std::runtime_error("Error. Complex is not a pseudo-manifold, which is necessary for option levels != 1.");
   }

   // count arcs and fill up maps
   std::vector<Int> varLevel;
   varLevel.reserve(M.edges()); // maybe a little bit too much, but more efficient than allocating incrementally
   Int m = 0;          // number of arcs
   Int numFaces = 0;   // number of faces
   for (Int k = 0; k < d; ++k) 
      for (auto f = entire(M.nodes_of_rank(k+1)); !f.at_end(); ++f, ++numFaces) 
         for (auto e = entire(M.out_edges(*f)); !e.at_end(); ++e, ++m)
            varLevel.push_back(k);
   
#if POLYMAKE_DEBUG
   if (debug_print)
      cout << "Dimension of complex:  " << d << "\n"
              "Faces in complex:      " << numFaces << "\n"
              "Arcs in Hasse Diagram: " << m << "\n"
              "Bottom level:          " << bottomLevel << "\n"
              "Top level:             " << topLevel << endl;
#endif


   // run greedy algorithm if requested
   if (runGreedy) {
#if POLYMAKE_DEBUG
      if (debug_print) cout << "\nComputing Morse matching by greedy heuristic ..." << endl;
#endif
      // compute lexicographic order
      std::vector<Int> varOrder;
      varOrder.reserve(M.edges());
      orderEdgesLex(M, varOrder, bottomLevel, topLevel);

      // compute greedy solution
      size = greedyHeuristic(M, EM, varLevel, varOrder.begin(), varOrder.end());
      assert( checkMatching(M, EM) );
      assert( checkAcyclic(M, EM) );

#if POLYMAKE_DEBUG
      if (debug_print) {
         cout << "Found solution of size:   " << size << endl;
         print_reversed_edges(M, EM);
         cout << "Number of critical faces: " << numFaces-2*size << endl;
      }
#endif
   }

   // run canceling algorithm if requested
   if (runCancel) {
#if POLYMAKE_DEBUG
      if (debug_print) cout << "\nTrying to improve Morse matching by canceling algorithm ..." << endl;
#endif
      processAlternatingPaths(M, EM, size, bottomLevel, topLevel);
      assert( checkMatching(M, EM) );
      assert( checkAcyclic(M, EM) );

#if POLYMAKE_DEBUG
      if (debug_print) {
         cout << "\nFound solution of size:      " << size << endl;
         print_reversed_edges(M, EM);
         cout << "Number of critical faces:    " << numFaces-2*size << endl;
      }
#endif
   }

   // complete solution to top level
   if (topLevel < d) {
#if POLYMAKE_DEBUG
      if (debug_print) cout << "\nCompleting to top level ..." << endl;
#endif
      completeToTopLevel(M, EM);
      assert( checkMatching(M, EM) );
      assert( checkAcyclic(M, EM) );

#if POLYMAKE_DEBUG
      if (debug_print) {
         size = count_marked_edges(EM);
         cout << "Found solution of size:   " << size << endl;
         print_reversed_edges(M, EM);
         cout << "Number of critical faces: " << numFaces-2*size << endl;
      }
#endif
   }

   // complete solution to bottom level
   if (bottomLevel > 0) {
#if POLYMAKE_DEBUG
      if (debug_print) cout << "\nCompleting to bottom level ..." << endl;
#endif
      completeToBottomLevel(M, EM);
      assert( checkMatching(M, EM) );
      assert( checkAcyclic(M, EM) );

#if POLYMAKE_DEBUG
      if (debug_print) {
         size = count_marked_edges(EM);
         cout << "Found solution of size:   " << size << endl;
         print_reversed_edges(M, EM);
         cout << "Number of critical faces: " << numFaces-2*size << endl;
      }
#endif
   }

   return EM;
}

Function4perl(&morse_matching, "morse_matching($ { heuristic => 0, levels => 0 })");

} }

// Local Variables: 
// mode:C++ 
// c-basic-offset:3 
// indent-tabs-mode:nil 
// End: 
