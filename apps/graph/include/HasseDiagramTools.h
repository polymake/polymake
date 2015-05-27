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

#ifndef POLYMAKE_GRAPH_HASSE_DIAGRAM_TOOLS_H
#define POLYMAKE_GRAPH_HASSE_DIAGRAM_TOOLS_H

#include "polymake/Set.h"
#include "polymake/graph/HasseDiagram.h"
#include "polymake/graph/BFSiterator.h"


namespace polymake { namespace graph {

int find_vertex_node(const HasseDiagram& HD, int v);

template <typename SetTop>
int find_face_node(const HasseDiagram& HD, const GenericSet<SetTop,int>& f, int dim=-1)
{
   for (Entire<HasseDiagram::nodes_of_dim_set>::const_iterator it=entire(HD.nodes_of_dim(dim)); !it.at_end(); ++it)
      if (HD.face(*it).front()==f)
         return *it;
   throw no_match("face node not found");
}

class HasseDiagram_facet_iterator
   : public BFSiterator< Graph<Directed> > {
   typedef BFSiterator< Graph<Directed> > super;
protected:
   const HasseDiagram *HD;
   int top_node;

   void valid_position()
   {
      int n;
      while (n=*(*this), HD->out_adjacent_nodes(n).front() != top_node)
         super::operator++();
   }
public:
   typedef HasseDiagram_facet_iterator iterator;
   typedef HasseDiagram_facet_iterator const_iterator;

   HasseDiagram_facet_iterator() : HD(0) {}

   HasseDiagram_facet_iterator(const HasseDiagram& HD_arg)
      : super(HD_arg.graph(), HD_arg.bottom_node()), HD(&HD_arg), top_node(HD_arg.top_node())
   {
      if (!at_end() && *(*this)!=top_node) valid_position();
   }

   HasseDiagram_facet_iterator(const HasseDiagram& HD_arg, int start_node)
      : super(HD_arg.graph(), start_node), HD(&HD_arg), top_node(HD_arg.top_node())
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

typedef bool2type<true> Down;
typedef bool2type<false> Up;

template<typename Direction>
Set<int> order_ideal(const Set<int>& generators, const graph::HasseDiagram& LF)
{
   Set<int> queue(generators), order_ideal; // make the queue a Set because any given element will be inserted lots of times
   while(queue.size()) {
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
bool is_convex_subset(const Set<int>& Cset, const HDType& LF, bool verbose) {
   // prepare down-sets and up-sets
   std::vector<Set<int> > down_set_of(LF.nodes()), up_set_of(LF.nodes());
   for (Entire<Set<int> >::const_iterator cit = entire(Cset); !cit.at_end(); ++cit) {
      down_set_of[*cit] = order_ideal<Down>(scalar2set(*cit), LF);
      up_set_of  [*cit] = order_ideal<Up>(scalar2set(*cit), LF);
   }

   // check convexity
   for (int d1=1; d1 <= LF.dim()-2; ++d1) {
      for (typename Entire<typename HDType::nodes_of_dim_set>::const_iterator d1it = entire(LF.nodes_of_dim(d1)); !d1it.at_end(); ++d1it) {
         if (!Cset.contains(*d1it)) continue;
         for (int d2=d1+2; d2 <= LF.dim(); ++d2) {
            for (typename Entire<typename HDType::nodes_of_dim_set>::const_iterator d2it = entire(LF.nodes_of_dim(d2)); !d2it.at_end(); ++d2it) {
               if (!Cset.contains(*d2it)) continue;
               const Set<int> interval = up_set_of[*d1it] * down_set_of[*d2it];
               if (incl(interval, Cset) > 0) {
                  if (verbose) cout << "The given set is not convex because "
                                    << "it contains " << *d1it << "=" << LF.face(*d1it) 
                                    << " and " << *d2it << "=" << LF.face(*d2it)
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

} } // end namespaces

namespace pm {
template <>
struct check_iterator_feature< polymake::graph::HasseDiagram_facet_iterator, end_sensitive > : True {};
}

#endif // POLYMAKE_GRAPH_HASSE_DIAGRAM_TOOLS_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
