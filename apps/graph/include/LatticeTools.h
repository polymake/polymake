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

#ifndef POLYMAKE_GRAPH_LATTICE_TOOLS_H
#define POLYMAKE_GRAPH_LATTICE_TOOLS_H

#include "polymake/Set.h"
#include "polymake/graph/Lattice.h"
#include "polymake/graph/graph_iterators.h"


namespace polymake { namespace graph {

template <typename LType>
int
find_vertex_node(const LType& HD, int v) {
   for (const auto n: HD.nodes_of_rank(1))
      if (HD.face(n).front()==v)
         return n;
   throw no_match("vertex node not found");
}

template <typename LType, typename SetType>
int
find_facet_node(const LType& HD, const GenericSet<SetType>& F) {
   for (const auto& f: HD.nodes_of_rank(HD.rank()-1))
      if (HD.face(f)==F)
         return f;
   throw no_match("facet node not found");
}
   
template <typename LType>
class HasseDiagram_facet_iterator
   : public BFSiterator< Graph<Directed> > {
   using base_t = BFSiterator< Graph<Directed> >;
protected:
   const LType *HD;
   int top_node;

   void valid_position()
   {
      int n;
      while (n=*(*this), HD->out_adjacent_nodes(n).front() != top_node)
      base_t::operator++();
   }
public:
   using iterator = HasseDiagram_facet_iterator;
   using const_iterator = HasseDiagram_facet_iterator;

   HasseDiagram_facet_iterator() : HD(nullptr) {}

   HasseDiagram_facet_iterator(const LType& HD_arg)
    : base_t(HD_arg.graph(), HD_arg.bottom_node())
    , HD(&HD_arg)
    , top_node(HD_arg.top_node())
   {
      if (!at_end() && *(*this)!=top_node) valid_position();
   }

   HasseDiagram_facet_iterator(const LType& HD_arg, int start_node)
    : base_t(HD_arg.graph(), start_node)
    , HD(&HD_arg)
    , top_node(HD_arg.top_node())
   {
      if (!at_end() && *(*this)!=top_node) valid_position();
   }

   iterator& operator++()
   {
      queue.pop_front();
      if (!at_end()) valid_position();
      return *this;
   }
   const iterator operator++(int) { iterator copy(*this); operator++(); return copy; }

   const Set<int>& face() const { return HD->face(*(*this)); }
   const Graph<Directed>& graph() const { return HD->graph(); }
   const Set<int>& face(int n) const { return HD->face(n); }
};

using Down = std::true_type;
using Up = std::false_type;

template<typename Direction, typename LType>
Set<int> order_ideal(const Set<int>& generators, const LType& LF)
{
   Set<int> queue(generators), order_ideal; // make the queue a Set because any given element will be inserted lots of times
  while (!queue.empty()) {
      const int s(queue.front());
      queue -= s;
      order_ideal += s;
      if (Direction::value)
         queue += LF.in_adjacent_nodes(s);
      else
         queue += LF.out_adjacent_nodes(s);
   }
   return order_ideal;
}

template<typename HDType>
bool is_convex_subset(const Set<int>& Cset, const HDType& LF, bool verbose)
{
   // prepare down-sets and up-sets
   std::vector<Set<int> > down_set_of(LF.nodes()), up_set_of(LF.nodes());
   for (const auto& c: Cset) {
      down_set_of[c] = order_ideal<Down>(scalar2set(c), LF);
      up_set_of  [c] = order_ideal<Up>  (scalar2set(c), LF);
   }

   // check convexity
   for (int d1=1; d1 <= LF.rank()-2; ++d1) {
      for (const auto d1node: LF.nodes_of_rank(d1)) {
         if (!Cset.contains(d1node)) continue;

         for (int d2=d1+2; d2 <= LF.rank(); ++d2) {
            for (const auto d2node: LF.nodes_of_rank(d2)) {
               if (!Cset.contains(d2node)) continue;
               const Set<int> interval = up_set_of[d1node] * down_set_of[d2node];
               if (incl(interval, Cset) > 0) {
                  if (verbose) cout << "The given set is not convex because "
                                    << "it contains " << d1node << "=" << LF.face(d1node)
                                    << " and " << d2node << "=" << LF.face(d2node)
                                    << ", but not the faces indexed by " << interval - Cset << " which are contained in the interval between them."
                                    << endl;
                  return false;
               }
            }
         }
      }
   }
   return true;
}

} }

namespace pm {
   template <typename Decoration>
   struct check_iterator_feature<polymake::graph::HasseDiagram_facet_iterator<Decoration>, end_sensitive> : std::true_type {};
}

#endif

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
