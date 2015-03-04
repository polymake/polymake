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
#include "polymake/graph/HasseDiagram.h"

namespace polymake { namespace graph {

void HasseDiagram::update_dim_after_squeeze()
{
   std::vector<int>::iterator map_begin=dim_map.begin(), map_end=dim_map.end();
   const int n=G.nodes()-1;
   std::vector<int>::iterator d=map_end-2;
   while (d >= map_begin && *d==n) --d;
   dim_map.erase(d+1, map_end-1);
   d=map_begin+1;
   while (d < map_end && *d==1) ++d;
   dim_map.erase(map_begin+1, d);
   count_map.clear();
}

void HasseDiagram::delete_node(int n)
{
   if (!G.has_gaps()) {
      int d=dim_map.size()-1;
      count_map.resize(d);
      while (--d>=0) count_map[d]=dim_map[d+1]-dim_map[d];
   }
   G.delete_node(n);
   int d=(std::upper_bound(dim_map.begin(), dim_map.end(), n) - dim_map.begin())-1;
   if (--count_map[d] == 0) {
      // the whole layer has gone
      int limit=count_map.size()-1;
      const bool bd=built_dually();
      if (bd) {
         if (d==0) {
            while (++d<=limit && !count_map[d]) {}
            count_map.erase(count_map.begin(), count_map.begin()+d);
            dim_map.erase(dim_map.begin(), dim_map.begin()+d);
         }
      } else {
         if (d==limit) {
            while (--d>=0 && !count_map[d]) {}
            ++d;
            count_map.resize(d);
            dim_map.erase(dim_map.begin()+d, dim_map.end()-1);
         }
      }
   }
}

Array< Set<int> > HasseDiagram::dual_faces() const
{
   Array< Set<int> > df(nodes());
   int i=0;
   for (Entire<nodes_of_dim_set>::const_iterator f=entire(nodes_of_dim(-1)); !f.at_end(); ++f, ++i)
      df[*f]+=i;
   for (int d=-2, bottom=-dim(); d>=bottom; --d)
      for (Entire<nodes_of_dim_set>::const_iterator f=entire(nodes_of_dim(d)); !f.at_end(); ++f)
         for (Entire< graph_type::out_adjacent_node_list >::const_iterator nb=entire(out_adjacent_nodes(*f));
              !nb.at_end(); ++nb)
            df[*f]+=df[*nb];
   return df;
}

perl::Object HasseDiagram::makeObject() const
{
   perl::Object fl("FaceLattice");
   fl.take("ADJACENCY") << graph();
   fl.take("FACES") << faces();
   fl.take("DIMS") << dims();
   return fl;
}

void operator<< (const perl::Value& v, const HasseDiagram& me)
{
   v << me.makeObject();
}

void HasseDiagram::fromObject(const perl::Object& fl)
{
   fl.give("ADJACENCY") >> G;
   fl.give("FACES") >> F;
   fl.give("DIMS") >> dim_map;
   built_min_first = (G.in_degree(0) == 0);
}

bool operator>> (const perl::Value& v, HasseDiagram& HD)
{
   perl::Object fl;
   v >> fl;
   if ((v.get_flags() & pm::perl::value_not_trusted) && !fl.isa("FaceLattice"))
      throw std::runtime_error("wrong object type for HasseDiagram");
   HD.fromObject(fl);
   return true;
}

int find_vertex_node(const HasseDiagram& HD, int v)
{
   if (HD.built_dually()) {
      for (Entire<HasseDiagram::nodes_of_dim_set>::const_iterator it=entire(HD.nodes_of_dim(0)); !it.at_end(); ++it)
         if (HD.face(*it).front()==v)
            return *it;
   } else {
      const sequence vertices=HD.node_range_of_dim(0);
      if (v>=0 && v<vertices.size())
         return vertices.front()+v;
   }
   throw no_match("vertex node not found");
}

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
