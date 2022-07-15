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

#pragma once

#include "polymake/client.h"
#include "polymake/Array.h"
#include "polymake/Set.h"
#include "polymake/Graph.h"
#include "polymake/Bitset.h"
#include "polymake/graph/graph_iterators.h"
#include "polymake/graph/arc_linking.h"

/** @file all_spanningtrees.h
 *  Algorithm for generating all spanning trees of an undirected connected graph along the lines of.
 *     Donald E. Knuth,
 *       in:
 *     The Art of Computer Programming,
 *     Volume 4, Fascicle 4, 24-31, 2006, Pearson Education Inc.
 */

namespace polymake { namespace graph {
   
template <typename TGraph>
std::pair<Set<Int>, Array<std::pair<Int,Int>>> initial_spanningtree(const TGraph& G)
{
   Array<std::pair<Int, Int>> dictionary(G.edges());
   Int k = 0;
   for (auto eit = entire(edges(G)); !eit.at_end(); ++eit) {
      dictionary[k] = std::pair<Int,Int>(eit.from_node(), eit.to_node());
      k++;
   }
   Set<Int> st;
   BFSiterator<TGraph> BFS_spanningtree(G, 0);
   Bitset visited; 
   do {
      visited =  BFS_spanningtree.node_visitor().get_visited_nodes();
      Int current = *BFS_spanningtree;
      ++BFS_spanningtree;
      visited ^=  BFS_spanningtree.node_visitor().get_visited_nodes();
      for (auto it = visited.begin(); !it.at_end(); ++it) {
         Int i = 0;
         for (auto eit = entire(edges(G)); !eit.at_end(); ++eit) {
            if (eit.to_node() == std::min(current,*it) && eit.from_node() == std::max(current,*it) )
               break;
            else ++i;
         }
         st += i;
      }
   } while (!BFS_spanningtree.at_end());
   return {st, dictionary};
}

std::pair<Array<Set<Int>>, Array<std::pair<Int,Int>>> all_spanningtrees(const Graph<>& G)
{  
   typedef ArcLinking IM;
   typedef ArcLinking::IncidenceCell IC;
   typedef ArcLinking::ColumnObject CO;
   
   std::list<Set<Int>> st;
   Array<IC*> arcs_by_id(G.edges());
   IM ArcGraph(G, arcs_by_id);

   //initialize variables for the algorithm
   Int n = G.nodes();
   Array<IC*> a(n-1), s(n-2);   
   if (n > 2)
      s[0] = nullptr;
   Array<Int> b(n,-1);
   auto initial_data = initial_spanningtree<>(G);
   Set<Int> init_st = initial_data.first;
   Int st_index = 0;
   for (auto it = entire(init_st); !it.at_end(); ++it, ++st_index) {
      a[st_index] = arcs_by_id[*it];
   }
   Int l = 0;                                 
   bool terminate = false, advancing = false, revert = false;
   CO *u = nullptr, *v = nullptr;
   IC *e = nullptr;

   //execute the steps of the algorithm
   while (!terminate) {
      if (n > 2) {
         e = a[l+1]; 
         u = ArcGraph.get_column_object(e->tip);
         v = ArcGraph.get_column_object(ArcGraph.reverse(e)->tip);
         if (u->size > v->size) {
            std::swap(u,v);
            e = ArcGraph.reverse(e);
         } 
         e->link = ArcGraph.contract_edge(u,v);
         a[l] = e;
         ++l;
         if (l<n-2) {
            s[l] = nullptr;
            advancing = true;
         }
         else {
            e = static_cast<IC*>(v->down);
            advancing = false;
         }
      }
      else {
         v = ArcGraph.get_column_object(1);            
         e = static_cast<IC*>(ArcGraph.get_column_object(0)->down);
      }
      if (!advancing) {
         Set<Int> fixed_edges;
         for (Int j = 0; j < n-2; ++j) {
               fixed_edges += a[j]->id;
         }
         Set<Int> completing_edges = ArcGraph.ids_of_column(ArcGraph.get_column_object(ArcGraph.reverse(e)->tip));
         for (auto it = entire(completing_edges); !it.at_end(); ++it) {
            fixed_edges += *it;
            st.push_back(fixed_edges);
            fixed_edges -= *it;
         } 
         a[n-2] = static_cast<IC*>(v->up);
         revert = true;
      }
      while (revert && !terminate) {
         --l;
         if (l == -1)
            terminate = true;
         else {
            e = a[l];
            u = ArcGraph.get_column_object(e->tip);
            v = ArcGraph.get_column_object(ArcGraph.reverse(e)->tip);
         }
         if (!terminate) {
            ArcGraph.expand_edge(u,v,e->link);
            bool bridge = true, exhausted = false;
            Int w = u->id;
            b[w] = v->id;
            while (bridge && !exhausted) {
               auto cit = u->begin();
               while (cit != u->end() && bridge) {
                  bool marked = false;
                  Int z = (*cit)->tip;
                  if (b[z] != -1) 
                     marked = true;
                  if (!marked && z != v->id) {
                     b[z] = v->id;
                     b[w] = z;
                     w = z;
                     marked = true;
                  }
                  if (!marked && static_cast<IC*>(*cit) != ArcGraph.reverse(e))   
                     bridge = false;    
                  if (bridge)
                     ++cit;
               }
               if (bridge) {
                  u = ArcGraph.get_column_object(b[u->id]);
                  if (u == v) {
                     exhausted = true;
                  }
               }
            }
            u = ArcGraph.get_column_object(e->tip);
            while (u != v) {
               w = b[u->id];
               b[u->id] = -1;
               u = ArcGraph.get_column_object(w);
            }
            if (bridge) {
               e = s[l];
               while (e != nullptr) {
                  u = ArcGraph.get_column_object(e->tip);
                  v = ArcGraph.get_column_object(ArcGraph.reverse(e)->tip);
                  ArcGraph.undelete_row(ArcGraph.reverse(e));
                  e = e->link;
               }
            }
            else {
               u = ArcGraph.get_column_object(e->tip);
               e->link = s[l];
               s[l] = e;
               ArcGraph.delete_row(e);
               revert = false;
            }
         }
      }
   }
   return {Array<Set<Int>>(st), initial_data.second};
}

} }


// Local Variables:
// c-basic-offset:3
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
