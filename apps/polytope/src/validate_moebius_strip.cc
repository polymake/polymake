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
   Set<int> face;

public:
   MissingFace(const Set<int>& S) : face(S) {}

   Set<int> get_face() const { return face; }
};

/** Searches for a given $k$-face in the face lattice and
 * throws a MissingFace exception if the face is missing.
 *
 * @complexity linear in the number of $k$-faces
 *
 * @throws MissingFace
 */
void check_k_face(const Set<int>& face, const int k, const Lattice<BasicDecoration, Sequential>& HD)
{
   for (auto f=entire(HD.nodes_of_rank(k+1)); !f.at_end(); ++f)
      if (HD.face(*f) == face)
         return; // found !!!

   throw MissingFace(face);
}

/** Searches for a given edge in the face lattice and
 * throws a MissingFace exception if the face is missing.
 *
 * @throws MissingFace
 */
inline void check_edge(const int u, const int v, const Lattice<BasicDecoration, Sequential>& HD)
{
   Set<int> S=scalar2set(u);
   S+=v;
   check_k_face(S,1,HD);
}

/** Searches for a given quadrangle (2-face) in the face lattice
 * and throws a MissingFace exception if it is missing.
 *
 * @throws MissingFace
 */
void check_quad (const int u1, const int u2,
                 const int u3, const int u4,
                 const Lattice<BasicDecoration, Sequential>& HD )
{
   Set<int> S=scalar2set(u1);
   S+=u2; S+=u3; S+=u4;
   assert(S.size()==4);
   check_k_face(S,2,HD);
}


/** Searches for all edges of a given quadrangle (2-face) in the face lattice
 * throws a MissingFace exception if some edge is missing.
 *
 * @throws MissingFace
 */
void check_quad_edges ( const int u1, const int u2,
                        const int u3, const int u4,
                        const Lattice<BasicDecoration, Sequential>& HD )
{
   check_edge(u1, u2, HD);
   check_edge(u2, u3, HD);
   check_edge(u3, u4, HD);
   check_edge(u4, u1, HD);
}


template <class OutIterator, class EdgeIterator>
int add_strip_edge(OutIterator& mse_it, EdgeIterator& strip_eit, EdgeMap<Undirected,int> &edges)
{
   edges[*strip_eit] = -2;

   Vector<int> e(2);

   const int head = strip_eit.from_node();
   e[0] = head;
   e[1] = strip_eit.to_node();

   *mse_it++ = e;
   return head;
}

} //end anonymous namespace


Matrix<int>  validate_moebius_strip_quads (perl::Object p, bool verbose)
{
   const Matrix<int> MS=p.give("MOEBIUS_STRIP_QUADS");
   const Lattice<BasicDecoration, Sequential> HD=p.give("HASSE_DIAGRAM");

   const int n = HD.nodes_of_rank(1).size();
   const int K = MS.rows();
   // --------------------------------------------------------
   if (verbose)
      cout << "Checking whether" << endl
           << "\t all quadrangles are in the face lattice .....";

   try {
      for (int i=0; i < K; ++i)
         check_quad(MS(i,0), MS(i,1), MS(i,2), MS(i,3), HD);
      if (verbose) cout << " yes"<< endl;

      // --------------------------------------------------------

      if (verbose) cout << "\t all edges are in the face lattice ...........";
      for (int i=0; i < K; ++i)
         check_quad_edges(MS(i,0), MS(i,1), MS(i,2), MS(i,3), HD);
      if (verbose) cout << " yes"<< endl;

      // --------------------------------------------------------

      Graph<> G(n);
      EdgeMap<Undirected,int> G_edges(G);

      for (int i=0; i < K; ++i) {
         ++G_edges(MS(i, 0), MS(i, 1));
         ++G_edges(MS(i, 1), MS(i, 2));
         ++G_edges(MS(i, 2), MS(i, 3));
         ++G_edges(MS(i, 3), MS(i, 0));
      }

      // --------------------------------------------------------
      if (verbose)
         cout << "Checking whether" << endl
              << "\t vertex degrees are in {0, 3, 4} .....";

      Set<int> valid_degrees;
      valid_degrees += 0;
      valid_degrees += 3;
      valid_degrees += 4;

      int seen_vertices = 0;
      for (int i=0; i < n; ++i) {
         const int deg = G.degree(i);
         if (deg > 0) ++seen_vertices;

         if (!valid_degrees.exists(deg)) {
            if (verbose)
               cout << "\n\t" << "vertex " << i << " has degree " << deg << endl;
            return Matrix<int>();
         }
      }
      if (verbose) cout << " yes"<< endl;

      Matrix<int> moebius_strip_edges(MS.rows(),2);
      Entire< Rows<Matrix<int> > >::iterator mse_it = entire(rows(moebius_strip_edges));

      // Search first edge
      typedef Graph<>::out_edge_list::iterator edge_iterator;

      int head = MS(0,0);
      const int start = head;
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
      return Matrix<int>();
   }
}



bool validate_moebius_strip(perl::Object p)
{
   const Matrix<int> MS=p.give("MOEBIUS_STRIP_EDGES");
   const Lattice<BasicDecoration, Sequential> HD=p.give("HASSE_DIAGRAM");

   // #quadrangles - 1
   const int K = MS.rows()-1;

   cout << "Checking whether" << endl
        << "\t all quadrangles are in the face lattice .....";
   // check all quadrangles but the last
   try {
      for (int i=0; i < K; ++i)
         check_quad(MS(i,0), MS(i,1), MS(i+1,1), MS(i+1,0), HD);

      // check the last one
      check_quad(MS(0,0), MS(0,1), MS(K,0), MS(K,1), HD);
      //                           ^^^^^^^^^^^^^^^^ reversed !!!!
      cout << " yes"<< endl;


      cout << "\t all edges are in the face lattice ...........";
      // check all quadrangles but the last
      for (int i=0; i < K; ++i)
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
