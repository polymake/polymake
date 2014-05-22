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

#ifndef POLYMAKE_GRAPH_BFS_ITERATOR_H
#define POLYMAKE_GRAPH_BFS_ITERATOR_H

#include "polymake/GenericGraph.h"
#include "polymake/Bitset.h"
#include "polymake/list"
#include "polymake/vector"
#include <cassert>

namespace polymake { namespace graph {

template <typename Dist=int>
class NodeVisitor {
protected:
   std::vector<Dist> dist;
public:
   NodeVisitor() {}

   template <typename Graph>
   NodeVisitor(const GenericGraph<Graph>& G, int start_node)
      : dist(G.top().dim(), Dist(-1))
   {
      if (!dist.empty()) dist[start_node]=0;
   }

   template <typename Graph>
   void reset(const GenericGraph<Graph>&, int start_node)
   {
      fill(pm::entire(dist), Dist(-1));
      if (!dist.empty()) dist[start_node]=0;
   }

   bool seen(int n) const { return dist[n]>=0; }
   void add(int n, int n_from) { dist[n]=dist[n_from]+1; }

   static const bool check_edges=false;
   void check(int,int) {}

   const Dist& operator[] (int n) const { return dist[n]; }
};

template <bool _inversed=false>
class BoolNodeVisitor {
protected:
   Bitset visited;
   int n_nodes;
public:
   BoolNodeVisitor() {}

   template <typename Graph>
   BoolNodeVisitor(const GenericGraph<Graph>& G, int start_node)
      : visited(G.top().dim(), _inversed && !G.top().has_gaps()), n_nodes(G.nodes())
   {
      if (_inversed && G.top().has_gaps()) visited=nodes(G);
      if (G.top().dim()) add(start_node);
   }

   template <typename Graph>
   void reset(const GenericGraph<Graph>& G, int start_node)
   {
      if (_inversed) {
         if (G.top().has_gaps())
            visited=nodes(G);
         else
            visited=sequence(0,n_nodes);
         visited-=start_node;
      } else {
         visited.clear();
         visited+=start_node;
      }
   }

   bool seen(int n) const { return _inversed^visited.contains(n); }
   void add(int n, int=0) { if (_inversed) visited-=n; else visited+=n; }

   static const bool check_edges=false;
   void check(int,int) {}

   const Bitset& get_visited_nodes() const { return visited; }
};

template <typename> class Visitor;
template <typename> class Reversed;

template <typename Graph, typename Params=void>
class BFSiterator {
public:
   typedef typename pm::extract_type_param<Params, Visitor, BoolNodeVisitor<> >::type visitor_type;
   static const bool reverse_edges=pm::extract_bool_param<Params,Reversed>::value;
protected:
   const Graph *graph;
   std::list<int> queue;
   visitor_type visitor;
   int unvisited;

   template <typename EdgeList>
   void next_step(int n, const EdgeList& edge_list)
   {
      for (typename Entire<EdgeList>::const_iterator e=entire(edge_list); !e.at_end(); ++e) {
         const int nn= reverse_edges ? e.from_node() : e.to_node();
         if (visitor.seen(nn)) {
            if (visitor.check_edges) visitor.check(nn,n);
         } else {
            visitor.add(nn,n);
            queue.push_back(nn);
            --unvisited;
         }
      }
   }

public:
   typedef std::forward_iterator_tag iterator_category;
   typedef int value_type;
   typedef const int& reference;
   typedef const int* pointer;
   typedef ptrdiff_t difference_type;

   typedef BFSiterator iterator;
   typedef BFSiterator const_iterator;

   BFSiterator() : graph(0) {}

   BFSiterator(const Graph& graph_arg, int start_node)
      : graph(&graph_arg), visitor(graph_arg, start_node), unvisited(graph_arg.nodes()-1)
   {
      if (unvisited>=0) {
         if (POLYMAKE_DEBUG) {
            if (start_node<0 || start_node>=graph->dim())
               throw std::runtime_error("BFSiterator - start node out of range");
         }
         queue.push_back(start_node);
      }
   }

   reference operator* () const { return queue.front(); }
   pointer operator-> () const { return &queue.front(); }

   iterator& operator++ ()
   {
      const int n=queue.front();  queue.pop_front();
      if (visitor.check_edges || unvisited>0) {
         if (reverse_edges)
            next_step(n, graph->in_edges(n));
         else
            next_step(n, graph->out_edges(n));
      }
      return *this;
   }
   const iterator operator++ (int) { iterator copy(*this);  operator++();  return copy; }

   void skip_node() { queue.pop_front(); }

   int unvisited_nodes() const { return unvisited; }
   const visitor_type& node_visitor() const { return visitor; }
   reference last_unvisited() const { return queue.back(); }

   bool at_end() const { return queue.empty(); }

   bool operator== (const iterator& it) const
   {
      return at_end() ? it.at_end() : !it.at_end() && queue.front()==it.queue.front();
   }
   bool operator!= (const iterator& it) const { return !operator==(it); }

   void reset(int start_node)
   {
      const int dim=graph->dim();
      if (dim>0) {
         if (POLYMAKE_DEBUG) {
            if (start_node<0 || start_node>=dim)
               throw std::runtime_error("BFSiterator::reset - start node out of range");
         }
         queue.clear();
         visitor.reset(*graph,start_node);
         queue.push_back(start_node);
         unvisited=graph->nodes()-1;
      }
   }
};

} }

namespace pm {
   template <typename Graph, typename Params>
   struct check_iterator_feature< polymake::graph::BFSiterator<Graph,Params>, end_sensitive > : True {};
}

#endif // POLYMAKE_GRAPH_BFS_ITERATOR_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
