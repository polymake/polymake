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

#ifndef POLYMAKE_GRAPH_HASSE_DIAGRAM_TOOLS_H
#define POLYMAKE_GRAPH_HASSE_DIAGRAM_TOOLS_H

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
} }

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
