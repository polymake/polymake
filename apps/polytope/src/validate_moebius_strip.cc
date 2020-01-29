/* Copyright (c) 1997-2020
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
#include "polymake/graph/Decoration.h"
#include "polymake/Matrix.h"
#include <cassert>

namespace polymake { namespace polytope {

   using graph::Lattice;
   using graph::lattice::Sequential;
   using graph::lattice::BasicDecoration;

namespace {

/** Exception: a face is not contained in the face lattice. */
class MissingFace
{
   Set<Int> face;

public:
   MissingFace(const Set<Int>& S) : face(S) {}

   Set<Int> get_face() const { return face; }
};

/** Searches for a given $k$-face in the face lattice and
 * throws a MissingFace exception if the face is missing.
 *
 * @complexity linear in the number of $k$-faces
 *
 * @throws MissingFace
 */
void check_k_face(const Set<Int>& face, const Int k, const Lattice<BasicDecoration, Sequential>& HD)
{
   for (const auto f : HD.nodes_of_rank(k+1))
      if (HD.face(f) == face)
         return; // found !!!

   throw MissingFace(face);
}

/** Searches for a given edge in the face lattice and
 * throws a MissingFace exception if the face is missing.
 *
 * @throws MissingFace
 */
void check_edge(const Int u, const Int v, const Lattice<BasicDecoration, Sequential>& HD)
{
   const Set<Int> S{ u, v };
   check_k_face(S, 1, HD);
}

/** Searches for a given quadrangle (2-face) in the face lattice
 * and throws a MissingFace exception if it is missing.
 *
 * @throws MissingFace
 */
void check_quad(const Int u1, const Int u2,
                const Int u3, const Int u4,
                const Lattice<BasicDecoration, Sequential>& HD)
{
   const Set<Int> S{ u1, u2, u3, u4 };
   assert(S.size() == 4);
   check_k_face(S, 2, HD);
}


/** Searches for all edges of a given quadrangle (2-face) in the face lattice
 * throws a MissingFace exception if some edge is missing.
 *
 * @throws MissingFace
 */
void check_quad_edges(const Int u1, const Int u2,
                      const Int u3, const Int u4,
                      const Lattice<BasicDecoration, Sequential>& HD)
{
   check_edge(u1, u2, HD);
   check_edge(u2, u3, HD);
   check_edge(u3, u4, HD);
   check_edge(u4, u1, HD);
}


template <class OutIterator, class EdgeIterator>
Int add_strip_edge(OutIterator& mse_it, EdgeIterator& strip_eit, EdgeMap<Undirected, Int>& edges)
{
   edges[*strip_eit] = -2;

   Vector<Int> e(2);

   const Int head = strip_eit.from_node();
   e[0] = head;
   e[1] = strip_eit.to_node();

   *mse_it++ = e;
   return head;
}

} //end anonymous namespace


Matrix<Int> validate_moebius_strip_quads(BigObject p, bool verbose)
{
   const Matrix<Int> MS=p.give("MOEBIUS_STRIP_QUADS");
   const Lattice<BasicDecoration, Sequential> HD=p.give("HASSE_DIAGRAM");

   const Int n = HD.nodes_of_rank(1).size();
   const Int K = MS.rows();
   // --------------------------------------------------------
   if (verbose)
      cout << "Checking whether" << endl
           << "\t all quadrangles are in the face lattice .....";

   try {
      for (Int i = 0; i < K; ++i)
         check_quad(MS(i,0), MS(i,1), MS(i,2), MS(i,3), HD);
      if (verbose) cout << " yes"<< endl;

      // --------------------------------------------------------

      if (verbose) cout << "\t all edges are in the face lattice ...........";
      for (Int i = 0; i < K; ++i)
         check_quad_edges(MS(i,0), MS(i,1), MS(i,2), MS(i,3), HD);
      if (verbose) cout << " yes"<< endl;

      // --------------------------------------------------------

      Graph<> G(n);
      EdgeMap<Undirected, Int> G_edges(G);

      for (Int i = 0; i < K; ++i) {
         ++G_edges(MS(i, 0), MS(i, 1));
         ++G_edges(MS(i, 1), MS(i, 2));
         ++G_edges(MS(i, 2), MS(i, 3));
         ++G_edges(MS(i, 3), MS(i, 0));
      }

      // --------------------------------------------------------
      if (verbose)
         cout << "Checking whether" << endl
              << "\t vertex degrees are in {0, 3, 4} .....";

      const Set<Int> valid_degrees{ 0, 3, 4 };
      Int seen_vertices = 0;
      for (Int i = 0; i < n; ++i) {
         const Int deg = G.degree(i);
         if (deg > 0) ++seen_vertices;

         if (!valid_degrees.exists(deg)) {
            if (verbose)
               cout << "\n\t" << "vertex " << i << " has degree " << deg << endl;
            return Matrix<Int>();
         }
      }
      if (verbose) cout << " yes"<< endl;

      Matrix<Int> moebius_strip_edges(MS.rows(), 2);
      auto mse_it = entire(rows(moebius_strip_edges));

      // Search first edge
      typedef Graph<>::out_edge_list::iterator edge_iterator;

      Int head = MS(0, 0);
      const Int start = head;
      for (;;) {
         edge_iterator strip_eit = G.out_edges(head).begin();
         for (; !strip_eit.at_end() && G_edges[*strip_eit] != 2; ++strip_eit);
         if (strip_eit.at_end()) {
            strip_eit = G.out_edges(head).begin();
            for (; !strip_eit.at_end() && G_edges[*strip_eit] != -2 ; ++strip_eit);
            if (!strip_eit.at_end() && strip_eit.to_node() != start)
               if (verbose) cout << "Not a Moebius strip!" << endl;
            break; // exit loop !!!!!!!!
         }
         add_strip_edge(mse_it, strip_eit, G_edges);

         edge_iterator up_eit = G.out_edges(head).begin();
         for (;!up_eit.at_end() && G_edges[*up_eit]!=1;++up_eit);
         assert(!up_eit.at_end());

         G_edges[*up_eit] = -1;
         head = up_eit.to_node();
      }

      if (verbose) cout << "Answer: It is a Moebius strip." << endl;
      return moebius_strip_edges;
   }
   catch (const MissingFace& e) {
      cerr << "\n\tAnswer: It is not a Moebius strip."
         "\n\tMissing face " << e.get_face() << endl;
      return Matrix<Int>();
   }
}


bool validate_moebius_strip(BigObject p)
{
   const Matrix<Int> MS = p.give("MOEBIUS_STRIP_EDGES");
   const Lattice<BasicDecoration, Sequential> HD=p.give("HASSE_DIAGRAM");

   // #quadrangles - 1
   const Int K = MS.rows()-1;

   cout << "Checking whether" << endl
        << "\t all quadrangles are in the face lattice .....";
   // check all quadrangles but the last
   try {
      for (Int i = 0; i < K; ++i)
         check_quad(MS(i,0), MS(i,1), MS(i+1,1), MS(i+1,0), HD);

      // check the last one
      check_quad(MS(0,0), MS(0,1), MS(K,0), MS(K,1), HD);
      //                           ^^^^^^^^^^^^^^^^ reversed !!!!
      cout << " yes"<< endl;


      cout << "\t all edges are in the face lattice ...........";
      // check all quadrangles but the last
      for (Int i = 0; i < K; ++i)
         check_quad_edges(MS(i,0), MS(i,1), MS(i+1,1), MS(i+1,0), HD);

      // check the last one
      check_quad_edges(MS(0,0), MS(0,1), MS(K,0), MS(K,1), HD);
      //                                 ^^^^^^^^^^^^^^^^ reversed !!!!
      cout << " yes"<< endl;
   }
   catch (const MissingFace& e) {
      cerr << "\t" << "Missing face " << e.get_face()<< endl;
      return false;
   }
   return true;
}

UserFunction4perl("# @category Consistency check"
                  "# Checks whether the [[Polytope::MOEBIUS_STRIP_QUADS|MOEBIUS_STRIP_QUADS]]"
                  "# form a Moebius strip with parallel opposite edges."
                  "# Prints a message to stdout and"
                  "# returns the [[Polytope::MOEBIUS_STRIP_EDGES|MOEBIUS_STRIP_EDGES]]"
                  "# if the answer is affirmative."
                  "# @param Polytope P the given polytope"
                  "# @option Bool verbose print details"
                  "# @return Matrix<Int> the Moebius strip edges"
                  "# @author Alexander Schwartz",
                  &validate_moebius_strip_quads,"validate_moebius_strip_quads(Polytope; $=0)");

UserFunction4perl("# @category Consistency check"
                  "# Validates the output of the client [[edge_orientable]],"
                  "# in particular it checks whether the [[Polytope::MOEBIUS_STRIP_EDGES|MOEBIUS_STRIP_EDGES]]"
                  "# form a Moebius strip with parallel opposite edges."
                  "# Prints a message to stdout."
                  "# @param Polytope P the given polytope"
                  "# @return Bool 'true' if the Moebius strip edges form such a Moebius strip, 'false' otherwise"
                  "# @author Alexander Schwartz",
                  &validate_moebius_strip,"validate_moebius_strip(Polytope)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
