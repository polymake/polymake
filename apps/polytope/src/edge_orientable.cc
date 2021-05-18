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

#include "polymake/client.h"
#include "polymake/Array.h"
#include "polymake/Map.h"
#include "polymake/Matrix.h"
#include "polymake/Graph.h"
#include "polymake/vector"
#include "polymake/list"
#include "polymake/graph/Lattice.h"
#include "polymake/graph/Decoration.h"
#include <stack>
#include <algorithm>
#include <cassert>

namespace polymake { namespace polytope {

typedef graph::Lattice<graph::lattice::BasicDecoration, graph::lattice::Sequential> Lattice;

namespace {

class EdgeOrientationAlg {

   struct Edge {
      Int id;
      Int head;
      Int tail;
      Int parent_edge;

      Edge(Int arg_id, Int arg_head, Int arg_tail, Int arg_parent_edge = -1)
         : id(arg_id)
         , head(arg_head)
         , tail(arg_tail)
         , parent_edge(arg_parent_edge) {}
   };

   typedef std::stack<Edge, std::vector<Edge>> stack_type;

   enum orientation_type { NOT_ORIENTED=0, LEFT=1, RIGHT=-1 } ;

   const Lattice HD;
   const Int      m, first_edge_in_HD;
   Array<Int>     orientation;
   Array<Int>     parent_edge;
   stack_type     stack;
   std::list<Int> moebius_strip;

#if POLYMAKE_DEBUG
   bool debug_print;
#endif

public:
   // --------------------------------------------------------------
   EdgeOrientationAlg (const Lattice& arg_HD )
      :              HD (arg_HD),
                     m (HD.nodes_of_rank(2).size()),
                     first_edge_in_HD (HD.nodes_of_rank(2).front()),
                     orientation (m, static_cast<Int>(NOT_ORIENTED)),
                     parent_edge (m, -1)
   {
#if POLYMAKE_DEBUG
      debug_print = get_debug_level() > 1;
#endif
   }
   // --------------------------------------------------------------
   /** Orient all dual cycles. */
   bool orient_edges ()
   {
      for (Int ee = 0; ee < m; ++ee)
         if (orientation[ee] == NOT_ORIENTED)
            if (!orient_dual_cycle(ee, LEFT))
               return false;

      return true;
   }
   // --------------------------------------------------------------
   Matrix<Int> get_moebius_strip() const
   {
      Matrix<Int> M (moebius_strip.size(), 2);

      Int i = 0;
      for (const Int e : moebius_strip) {
         M(i, 0) = orientation[e] == LEFT ? HD.face(e+first_edge_in_HD).front()
            : HD.face(e+first_edge_in_HD).back();
         M(i, 1) = orientation[e] == LEFT ? HD.face(e+first_edge_in_HD).back()
            : HD.face(e+first_edge_in_HD).front();
         ++i;
      }

      return M;
   }
   // --------------------------------------------------------------
   Matrix<Int> get_edge_orientation() const
   {
      Matrix<Int> E(m, 2);

      for (Int i=0; i < m; ++i) {
         E(i,0) = orientation[i] == LEFT ? HD.face(i+first_edge_in_HD).front()
            : HD.face(i+first_edge_in_HD).back();

         E(i,1) = orientation[i] == LEFT ? HD.face(i+first_edge_in_HD).back()
            : HD.face(i+first_edge_in_HD).front();
      }

      return E;
   }

private:
   // --------------------------------------------------------------
   bool orient_dual_cycle(Int ee, Int orient)
   {
      set_edge_orientation(ee, orient);

      while (!stack.empty()) {
         const Edge e = stack.top(); stack.pop();

         for (auto q = entire(HD.out_edges(e.id+first_edge_in_HD)); !q.at_end(); ++q) {
#if POLYMAKE_DEBUG
            if (debug_print) cout << "Quad " << q.to_node() << " (" << HD.face(q.to_node()) << ")" << endl;
#endif
            const Edge opposite_edge = get_opposite_edge(e, entire(HD.in_edges(q.to_node())));

            if (!set_edge_orientation(opposite_edge))
               return false;
         }
      }

      return true;
   }
   // --------------------------------------------------------------
   bool set_edge_orientation(Edge e)
   {
      return set_edge_orientation(e.id,
                                  (e.head == HD.face(e.id+first_edge_in_HD).front()) ? LEFT : RIGHT, e.parent_edge);
   }
   // --------------------------------------------------------------
   Int same_orientation(Int p, Int orient)
   {
      return orientation[p] * orient;
   }
   // --------------------------------------------------------------
   bool set_edge_orientation(Int e, Int orient, Int p = -1)
   {
      assert(orient != NOT_ORIENTED);

      const Int head = orient == LEFT ? HD.face(e+first_edge_in_HD).front()
         : HD.face(e+first_edge_in_HD).back();
      const Int tail = orient == LEFT ? HD.face(e+first_edge_in_HD).back()
         : HD.face(e+first_edge_in_HD).front();


#if POLYMAKE_DEBUG
      if (debug_print) {
         cout << e << ": " << HD.face(e+first_edge_in_HD) << " --> " << orient;
         if (p != -1) cout << " (" << p << ")";
         cout << endl;
      }
#endif

      if (orientation[e] != NOT_ORIENTED && orientation[e] != orient) {
#if POLYMAKE_DEBUG
         if (debug_print) cout << "\t" << "CONFLICT" << endl;
#endif

         moebius_strip.push_back(e);
         process_parent_edges(e, parent_edge[e], std::back_inserter(moebius_strip));

         std::list<Int> tmp;
         process_parent_edges(e, p, std::front_inserter(tmp));
         tmp.pop_front();

         std::copy(tmp.begin(), tmp.end(), std::back_inserter(moebius_strip));

         return false;
      }

      if (orientation[e] != NOT_ORIENTED)
         return true; // nothing to do!

      stack.push(Edge(e, head, tail));
      if (p != -1)
         parent_edge[e] = p;

      orientation[e] = orient;

      return true;
   }
   // --------------------------------------------------------------
   template <class Graph, class EdgeIterator>
   EdgeIterator next_cycle_edge(Graph const& G, EdgeIterator eit_last)
   {
      assert(!eit_last.at_end());

      for (EdgeIterator eit = entire(G.out_edges(eit_last.to_node()));
           !eit.at_end(); ++eit)
         if (eit.to_node() != eit_last.from_node())
            return eit;

      assert(false);
      return EdgeIterator();
   }

   template <typename OutIterator>
   void process_parent_edges(Int e, Int p, OutIterator out_it)
   {
#if POLYMAKE_DEBUG
      if (debug_print) cout << "---- parent edges of " << e << ", parent_edge = " << p << endl;
#endif

      for (; p != -1; p = parent_edge[p]) {
         *(out_it++) = p;

#if POLYMAKE_DEBUG
         if (debug_print) cout << "\t" << p << ": " << HD.face(p+first_edge_in_HD) << endl;
#endif
      }

#if POLYMAKE_DEBUG
      if (debug_print) cout << endl;
#endif
   }
   // --------------------------------------------------------------
   template <typename EdgeIt>
   Edge get_opposite_edge(const Edge e, const EdgeIt eit_begin)
   {
#if POLYMAKE_DEBUG
      if (debug_print)
         cout << "\t#### get_opposite_edge(e=" << e.id
              << "(" << e.id+first_edge_in_HD << ")"
              << ", h=" << e.head << ", t=" << e.tail << ")" << endl;
#endif
      // Collect node numbers
      const Set<Int>& nodes = HD.face(eit_begin.to_node());

      assert(nodes.size() == 4);
      Graph<> G(nodes.size());
      NodeMap<Undirected, Int> nm(G);
      EdgeMap<Undirected, Int> em(G);

      // Create graph nodes
      Map<Int, Int> node_number;
      Int n = 0;
      for (auto v = entire(nodes); !v.at_end(); ++n, ++v) {
         node_number[*v] = n;
         nm[n] = *v;
      }

      // Create graph edges
      for (EdgeIt eit = eit_begin; !eit.at_end(); ++eit) {
         const Int tail = node_number[HD.face(eit.from_node()).front()];
         const Int head = node_number[HD.face(eit.from_node()).back()];

#if POLYMAKE_DEBUG
         if (debug_print) cout << "new edge: head=" << head << ", tail=" << tail << endl;
#endif

         em(head, tail)=eit.index();
      }

#if POLYMAKE_DEBUG
      if (debug_print) cout << "GRAPH" << endl << G << endl;
#endif
      // Get first cycle edge

      Graph<>::out_edge_list::const_iterator
         eit_first_edge = entire(G.out_edges(node_number[e.head]));
      for (; !eit_first_edge.at_end() && em[*eit_first_edge] != e.id+first_edge_in_HD;
           ++eit_first_edge)
         ; // OK!
      assert(!eit_first_edge.at_end() && em[*eit_first_edge] == e.id+first_edge_in_HD);

      Graph<>::out_edge_list::const_iterator
         eit = next_cycle_edge(G, next_cycle_edge(G, eit_first_edge));

      const Int tail = nm[eit.from_node()];
      const Int head = nm[eit.to_node()];

      return Edge(em[*eit]-first_edge_in_HD, head, tail, e.id);
   }

};

} // end unnamed namespace

void edge_orientable(BigObject p)
{
   const Int cubicality = p.give("CUBICALITY");
   if (cubicality < 2)
      throw std::runtime_error("2-cubical polytope expected");

   BigObject HD_obj =p.give("HASSE_DIAGRAM");
   const Lattice HD(HD_obj);
   EdgeOrientationAlg alg(HD);

   // main method
   const bool is_orientable = alg.orient_edges();

   p.take("EDGE_ORIENTABLE") << is_orientable;
   if (is_orientable)
      p.take("EDGE_ORIENTATION") << alg.get_edge_orientation();
   else
      p.take("MOEBIUS_STRIP_EDGES") << alg.get_moebius_strip();
}

UserFunction4perl("# @category Other"
                  "# Checks whether a 2-cubical polytope //P// is __edge-orientable__ "
                  "# (in the sense of Hetyei), that means that there exits an orientation "
                  "# of the edges such that for each 2-face the opposite edges point "
                  "# in the same direction."
                  "# It produces the certificates [[EDGE_ORIENTATION]] if the polytope is"
                  "# edge-orientable, or [[MOEBIUS_STRIP_EDGES]] otherwise."
                  "# In the latter case, "
                  "# the output can be checked with the client [[validate_moebius_strip]]."
                  "# @param Polytope P the given 2-cubical polytope"
                  "# @author Alexander Schwartz",
                  &edge_orientable,"edge_orientable");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
