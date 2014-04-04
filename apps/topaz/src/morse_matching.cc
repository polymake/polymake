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

#include "polymake/client.h"
#include "polymake/graph/HasseDiagram.h"
#include "polymake/Array.h"
#include "polymake/topaz/morse_matching_tools.h"
#include <cassert>


namespace polymake { namespace topaz {


HasseEdgeMap morse_matching(perl::Object p, perl::OptionSet options)
{
   typedef Graph<Directed>::out_edge_list::const_iterator HasseDiagramOutConstIterator;
   // determine which heuristic to run
   bool runGreedy = true;  // default is to turn both heuristics on
   bool runCancel = true;
   const int heuristic = options["heuristic"];
   if (heuristic == 1) runCancel = false;
   if (heuristic == 2) runGreedy = false;

#if POLYMAKE_DEBUG
   const bool debug_print = perl::get_debug_level() > 1;
   if (debug_print)
      cout << "heuristic: " << heuristic 
           << ", runGreedy=" << runGreedy 
           << ", runCancel=" << runCancel << endl;
#endif

   graph::HasseDiagram M = p.give("HASSE_DIAGRAM"); // not const, will be modified
   const int d = M.dim() - 1;
   int size = 0;

#if POLYMAKE_DEBUG
   if (debug_print) cout << "d: " << d << endl;
#endif

   // find lowest and highest levels
   int bottomLevel = 0;
   int topLevel = d;
   const int levels = options["levels"];
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

   HasseEdgeMap EM(M.graph());

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
   std::vector<int> varLevel;
   varLevel.reserve(M.edges()); // maybe a little bit too much, but more efficient than allocating incrementally
   int m = 0;          // number of arcs
   int numFaces = 0;   // number of faces
   for (int k = 0; k < d; ++k) 
      for (Entire<graph::HasseDiagram::nodes_of_dim_set>::const_iterator f = entire(M.nodes_of_dim(k)); !f.at_end(); ++f, ++numFaces) 
         for (HasseDiagramOutConstIterator e = entire(M.out_edges(*f)); !e.at_end(); ++e, ++m)
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
      std::vector<int> varOrder;
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
         cout << "Number of critical faces: " << numFaces - 2*size << endl;
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
         cout << "Number of critical faces:    " << numFaces - 2*size << endl;
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
         size = EdgeMapSize(EM);
         cout << "Found solution of size:   " << size << endl;
         print_reversed_edges(M, EM);
         cout << "Number of critical faces: " << numFaces - 2*size << endl;
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
         size = EdgeMapSize(EM);
         cout << "Found solution of size:   " << size << endl;
         print_reversed_edges(M, EM);
         cout << "Number of critical faces: " << numFaces - 2*size << endl;
      }
#endif
   }

   return EM;
}

UserFunction4perl("# @category Computing properties "
                  "#  Compute a Morse matching. Two heuristics are implemented: "
                  "# "
                  "#  - A simple greedy algorithm: "
                  "#    The arcs are visited in lexicographical order, i.e.: "
                  "# "
                  "#    we proceed by levels from top to bottom, "
                  "#    visit the faces in each dimension in lexicographical order, "
                  "#    and visited the faces covered by these faces in lexicographical order. "
                  "# "
                  "#    This heuristic is used by default and with heuristic => 1. "
                  "# "
                  "#  - A Morse matching can be improved by canceling critical cells "
                  "#    along unique alternating paths, see function "
                  "#    processAlternatingPaths() in file morse_matching_tools.h . "
                  "#    This idea is due to Robin Forman: "
                  "# "
                  "#        Morse Theory for Cell-Complexes, "
                  "#        Advances in Math., 134 (1998), pp. 90-145. "
                  "# "
                  "#    This heuristic is used by default and with heuristic => 2. "
                  "# "
                  "#  The default setting is to use both, i.e., to run the greedy algorithm "
                  "#  and then improve the result by the canceling algorithm. "
                  "# "
                  "#  Morse matchings for the bottom level can be found optimally by "
                  "#  spanning tree techniques. This can be enabled by the option "
                  "#  levels => 1.  If the complex is a pseudo-manifold the same can be "
                  "#  done for the top level (option levels => 2). By specifying option "
                  "#  levels => 0, both levels can be computed by spanning trees. "
                  "#  For 2-dim pseudo-manifolds this computes an optimal Morse matching. "
                  "# "
                  "# @param p SimplicialComplex given by its Hasse diagram "
                  "# @option Int heuristic (1=greedy, 2=cancel, 0=both (default)) "
                  "# @option Int levels    (1=bottom, 2=top, 0=both (default)) "
                  "# @return EdgeMap matching a labelling of the edges of the Hasse diagram with integer values, where 1 means that the edge is in the matching",
                  &morse_matching, "morse_matching($ { heuristic => 0, levels => 0 })");

} }

// Local Variables: 
// mode:C++ 
// c-basic-offset:3 
// indent-tabs-mode:nil 
// End: 
