/* Copyright (c) 1997-2023
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
#include "polymake/topaz/complex_tools.h"
#include "polymake/Graph.h"
#include "polymake/Bitset.h"
#include "polymake/PowerSet.h"
#include "polymake/hash_map"
#include <sstream>

namespace polymake { namespace topaz {

typedef std::pair<Int, Int> gen_label;
typedef std::list<std::pair<Int, Int>> relation;
typedef std::pair<Int, std::list<relation>> fp_group;

// compute a finite presentation of the fundamental group
void fundamental_group(BigObject p)
{
   const Array<Set<Int>> C=p.give("FACETS");
   const bool is_connected = p.give("GRAPH.CONNECTED");
   if (!is_connected)
      throw std::runtime_error("fundamental_group: Complex must be connected.");
   const Graph<> G=p.give("GRAPH.ADJACENCY");

   EdgeMap<Undirected, bool> marked(G,true);  // in the beginning all edges are marked

   const Int n_verts = G.nodes();
   Bitset connected_verts(n_verts);
   Int n_edges = 0; // number of unmarked edges
   
   // compute a spanning tree; in the end the non-tree edges are marked
   std::list<Int> queue;
   queue.push_back(0); // 0 is a node!
   connected_verts.insert(0);
   while (!queue.empty() && n_edges<n_verts-1) { // size of a spanning tree is known since G connected
      const Int node = queue.front();
      queue.pop_front();
      Set<Int> neighbors = G.adjacent_nodes(node);
      for (auto ni = entire(neighbors); !ni.at_end(); ++ni) {
         if (!connected_verts.contains(*ni)) {
            marked(node,*ni)=false; // edge in the spanning tree
            connected_verts.insert(*ni);
            queue.push_back(*ni);
            ++n_edges;
         }
      }
   }
   
   // each non-tree edge gives a generator
   hash_map<gen_label, Int> generators(n_edges);
   std::list<std::string> gen_labels;
   Int c = 0;
   for (auto e_it = entire(edges(G)); !e_it.at_end(); ++e_it) {
      if (marked[*e_it]) {
         std::ostringstream label;
         if (e_it.from_node() < e_it.to_node()) {
            const gen_label gl(e_it.from_node(),e_it.to_node());
            generators[gl] = c;
            label << "g" << e_it.from_node() << "_" << e_it.to_node();
         } else {
            const gen_label gl(e_it.to_node(),e_it.from_node());
            generators[gl] = c;
            label << "g" << e_it.to_node() << "_" << e_it.from_node();
         }
         gen_labels.push_back(label.str());
         ++c;
      }
   }
   
   // each 2-face gives a relation
   std::list<relation> relations;
   const PowerSet<Int> skeleton_2 = k_skeleton(C,2);
   for (auto c_it=entire(skeleton_2); !c_it.at_end(); ++c_it) {
      relation rel;
      for (auto s_it = entire(*c_it); !s_it.at_end(); ) {
         const Int from_node = *s_it;
         ++s_it;
         if (s_it.at_end()) {
            if (marked(c_it->front(),from_node)) {
               rel.emplace_back(generators[gen_label(c_it->front(),from_node)], -1);
            }
         } else {
            if (marked(from_node,*s_it)) {
               rel.emplace_back(generators[gen_label(from_node,*s_it)], 1);
            }
         }
      }
      if (rel.size()) relations.push_back(std::move(rel)); // omit empty relations
   }
   
   p.take("FUNDAMENTAL_GROUP") << fp_group(gen_labels.size(),relations);
   p.take("FUNDAMENTAL_GROUP_GEN_LABELS") << as_array(gen_labels);
}

Function4perl(&fundamental_group, "fundamental_group");   

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
