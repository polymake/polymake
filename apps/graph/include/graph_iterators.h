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

#pragma once

#include "polymake/GenericGraph.h"
#include "polymake/Bitset.h"
#include "polymake/ContainerChain.h"
#include "polymake/vector"
#include "polymake/meta_list.h"
#include <deque>
#include <cassert>

namespace polymake { namespace graph {

template <bool TInversed=false>
class NodeVisitor {
protected:
   Bitset visited;
public:
   static const bool visit_all_edges=false;

   NodeVisitor() = default;

   template <typename TGraph>
   NodeVisitor(const GenericGraph<TGraph>& G)
      : visited(G.top().dim())
   {
      clear(G);
   }

   template <typename TGraph>
   void clear(const GenericGraph<TGraph>& G)
   {
      if (TInversed) {
         if (G.top().has_gaps())
            visited=nodes(G);
         else
            visited=sequence(0, G.top().dim());
      } else {
         visited.clear();
      }
   }

   bool operator()(Int n)
   {
      return operator()(n, n);
   }

   bool operator()(Int n_from, Int n_to)
   {
      if (TInversed == visited.contains(n_to)) {
         if (TInversed)
            visited-=n_to;
         else
            visited+=n_to;
         return true;
      }
      return false;
   }

   const Bitset& get_visited_nodes() const { return visited; }
};


/// storage for and actions on visited nodes
template <typename> class VisitorTag;

/// how to traverse a directed graph:
/// 1 = follow the edges as is
/// -1 = follow reversed edges
/// 0 = follow all incident adges regardless of their direction
template <typename> class TraversalDirectionTag;

/// DFS only: whether to expose the parent (root) node before its children (leaves)
template <typename> class VisitParentFirstTag;

template <typename TGraph, typename... TParams>
class graph_iterator_base {
public:
   typedef typename mlist_wrap<TParams...>::type params;
   typedef TGraph graph_t;
   typedef typename mtagged_list_extract<params, VisitorTag, NodeVisitor<> >::type visitor_t;
   typedef typename std::conditional<TGraph::is_directed,
                                     typename mtagged_list_extract<params, TraversalDirectionTag, int_constant<1>>::type,
                                     int_constant<1>>::type traverse_edges;

   typedef std::forward_iterator_tag iterator_category;
   typedef Int value_type;
   typedef const Int& reference;
   typedef const Int* pointer;
   typedef ptrdiff_t difference_type;

   const visitor_t& node_visitor() const { return visitor; }
   visitor_t& node_visitor_mutable() { return visitor; }

protected:
   graph_iterator_base()
      : graph(nullptr) {}

   explicit graph_iterator_base(const graph_t& graph_arg)
      : graph(&graph_arg)
      , visitor(graph_arg)
      , undiscovered(graph->nodes()) {}

   graph_iterator_base(const graph_t& graph_arg, visitor_t&& visitor_arg)
      : graph(&graph_arg)
      , visitor(std::move(visitor_arg))
      , undiscovered(graph->nodes()) {}

   decltype(auto) edges(Int n, int_constant<1>) const
   {
      return graph->out_edges(n);
   }

   decltype(auto) edges(Int n, int_constant<-1>) const
   {
      return graph->in_edges(n);
   }

   decltype(auto) edges(Int n, int_constant<0>) const
   {
      return concatenate(graph->out_edges(n), graph->in_edges(n));
   }

public:
   decltype(auto) edges(Int n) const
   {
      return entire(edges(n, traverse_edges()));
   }

   /// get the number of nodes which haven't been touched so far
   Int undiscovered_nodes() const { return undiscovered; }

protected:
   void reset()
   {
      visitor.clear(*graph);
      undiscovered=graph->nodes();
   }

   template <typename TEdgeIterator>
   static
   Int from_node(const TEdgeIterator& e, int_constant<1>)
   {
      return e.from_node();
   }

   template <typename TEdgeIterator>
   static
   Int from_node(const TEdgeIterator& e, int_constant<-1>)
   {
      return e.to_node();
   }

   template <typename TEdgeIterator>
   static
   Int to_node(const TEdgeIterator& e, int_constant<1>)
   {
      return e.to_node();
   }

   template <typename TEdgeIterator>
   static
   Int to_node(const TEdgeIterator& e, int_constant<-1>)
   {
      return e.from_node();
   }

   template <typename TEdgeIterator>
   static
   Int from_node(const TEdgeIterator& e, int_constant<0>)
   {
      return e.get_leg()==0
             ? std::get<0>(e.get_it_tuple()).from_node()
             : std::get<1>(e.get_it_tuple()).to_node();
   }

   template <typename TEdgeIterator>
   static
   Int to_node(const TEdgeIterator& e, int_constant<0>)
   {
      return e.get_leg()==0
             ? std::get<0>(e.get_it_tuple()).to_node()
             : std::get<1>(e.get_it_tuple()).from_node();
   }

   template <typename TEdgeIterator>
   static
   Int from_node(const TEdgeIterator& e)
   {
      return from_node(e, traverse_edges());
   }

   template <typename TEdgeIterator>
   static
   Int to_node(const TEdgeIterator& e)
   {
      return to_node(e, traverse_edges());
   }

   template <typename TEdgeIterator>
   static
   std::false_type probe_visitor(const TEdgeIterator& e, decltype(std::declval<visitor_t&>()(e.from_node(), e.to_node())));

   template <typename TEdgeIterator>
   static
   std::true_type probe_visitor(const TEdgeIterator& e, decltype(std::declval<visitor_t&>()(e.from_node(), e.to_node(), *e)));

   typedef decltype(probe_visitor(entire(std::declval<graph_t>().out_edges(0)), true)) visitor_needs_edge;

   template <typename TEdgeIterator>
   typename std::enable_if<visitor_needs_edge::value, typename mproject2nd<TEdgeIterator, bool>::type>::type
   visit_edge(Int n_from, Int n_to, const TEdgeIterator& e)
   {
      return visitor(n_from, n_to, *e);
   }

   template <typename TEdgeIterator>
   typename std::enable_if<!visitor_needs_edge::value, typename mproject2nd<TEdgeIterator, bool>::type>::type
   visit_edge(Int n_from, Int n_to, const TEdgeIterator& e)
   {
      return visitor(n_from, n_to);
   }

   const graph_t *graph;
   visitor_t visitor;
   Int undiscovered;
};


template <typename TGraph, typename... TParams>
class BFSiterator
   : public graph_iterator_base<TGraph, TParams...> {
   using base_t = graph_iterator_base<TGraph, TParams...>;
public:
   using typename base_t::visitor_t;
   using typename base_t::reference;
   using queue_t = std::deque<Int>;

   using iterator = BFSiterator;
   using const_iterator = BFSiterator;

   BFSiterator() = default;

   explicit BFSiterator(const GenericGraph<TGraph>& graph_arg)
      : base_t(graph_arg.top()) {}

   BFSiterator(const GenericGraph<TGraph>& graph_arg, visitor_t&& visitor_arg)
      : base_t(graph_arg.top(), std::move(visitor_arg)) {}

   BFSiterator(const GenericGraph<TGraph>& graph_arg, Int start_node)
      : base_t(graph_arg.top())
   {
      process(start_node);
   }

   BFSiterator(const GenericGraph<TGraph>& graph_arg, visitor_t&& visitor_arg, Int start_node)
      : base_t(graph_arg.top(), std::move(visitor_arg))
   {
      process(start_node);
   }

   /// get the current node
   reference operator* () const { return queue.front(); }

   /// add the neighbors of the current node to the search front, then switch to the next node
   iterator& operator++ ()
   {
      const Int n = queue.front();  queue.pop_front();
      if (visitor_t::visit_all_edges || this->undiscovered != 0)
         propagate(n, this->edges(n));
      return *this;
   }
   const iterator operator++ (int) { iterator copy(*this);  operator++();  return copy; }

   /// switch to the next node without visiting neighbors of the current one
   void skip_node() { queue.pop_front(); }

   const queue_t& get_queue() const { return queue; }

   bool at_end() const { return queue.empty(); }

   bool operator== (const iterator& it) const
   {
      return at_end() ? it.at_end() : !it.at_end() && queue.front()==it.queue.front();
   }
   bool operator!= (const iterator& it) const { return !operator==(it); }

   /// restore the initial state of the iterator, make all nodes undiscovered
   void reset(Int start_node)
   {
      base_t::reset();
      restart(start_node);
   }

   /// empty the queue, make the given node the current one
   void restart(Int n)
   {
      queue.clear();
      process(n);
   }

   /// make the given node the current one without clearing the visited state of any nodes
   /// and preserving the queue.
   void process(Int n)
   {
      if (const Int dim = this->graph->dim()) {
         if (POLYMAKE_DEBUG) {
            if (n < 0 || n >= dim)
               throw std::runtime_error("BFSiterator - start node out of range");
         }
         if (this->visitor(n)) {
            queue.push_back(n);
            --this->undiscovered;
         }
      }
   }

protected:
   template <typename TEdgeIterator>
   void propagate(Int n, TEdgeIterator&& e)
   {
      for (; !e.at_end(); ++e) {
         const Int to_n = this->to_node(e);
         if (this->visit_edge(n, to_n, e)) {
            queue.push_back(to_n);
            --this->undiscovered;
         }
      }
   }

   queue_t queue;
};


template <typename TGraph, typename... TParams>
class DFSiterator
   : public graph_iterator_base<TGraph, TParams...> {
   using base_t = graph_iterator_base<TGraph, TParams...>;
public:
   using typename base_t::visitor_t;
   using typename base_t::reference;
   using typename base_t::params;

   static const bool visit_parent_first=tagged_list_extract_integral<params, VisitParentFirstTag>(false);

   using iterator = DFSiterator;
   using const_iterator = DFSiterator;

   DFSiterator()
      : cur(-1) {}

   explicit DFSiterator(const GenericGraph<TGraph>& graph_arg)
      : base_t(graph_arg.top())
      , cur(-1) {}

   DFSiterator(const GenericGraph<TGraph>& graph_arg, visitor_t&& visitor_arg)
      : base_t(graph_arg.top(), std::move(visitor_arg))
      , cur(-1) {}

   DFSiterator(const GenericGraph<TGraph>& graph_arg, Int start_node)
      : base_t(graph_arg.top())
      , cur(-1)
   {
      process(start_node);
   }

   DFSiterator(const GenericGraph<TGraph>& graph_arg, visitor_t&& visitor_arg, Int start_node)
      : base_t(graph_arg.top(), std::move(visitor_arg))
      , cur(-1)
   {
      process(start_node);
   }

   reference operator* () const { return cur; }

   iterator& operator++ ()
   {
      if (visit_parent_first) {
         it_stack.push_back(this->edges(cur));
         propagate();
      } else {
         cur=predecessor();
         if (cur >= 0) {
            ++it_stack.back();
            descend();
         }
      }
      return *this;
   }

   const iterator operator++ (int) { iterator copy(*this);  operator++();  return copy; }

   void skip_node()
   {
      assert(visit_parent_first && !it_stack.empty());
      ++it_stack.back();
      propagate();
   }

   bool at_end() const { return cur<0; }

   bool operator== (const iterator& it) const { return cur==it.cur; }
   bool operator!= (const iterator& it) const { return !operator==(it); }

   /// restore the initial state of the iterator, make all nodes undiscovered
   void reset(Int start_node)
   {
      base_t::reset();
      restart(start_node);
   }

   /// empty the stack, start from the given node
   void restart(Int n)
   {
      it_stack.clear();
      process(n);
   }

   void process(Int n)
   {
      if (const Int dim = this->graph->dim()) {
         if (POLYMAKE_DEBUG) {
            if (n < 0 || n >= dim)
               throw std::runtime_error("DFSiterator - start node out of range");
         }
         if (this->visitor(n)) {
            cur = n;
            --this->undiscovered;
            if (!visit_parent_first) {
               it_stack.push_back(this->edges(n));
               descend();
            }
         }
      }
   }

   using edge_iterator = decltype(std::declval<base_t>().edges(0));
   using it_stack_t = std::deque<edge_iterator>;

   const it_stack_t& get_stack() const { return it_stack; }

   Int predecessor() const { return it_stack.empty() ? -1 : base_t::from_node(it_stack.back()); }

protected:
   void descend()
   {
      while (!it_stack.back().at_end()) {
         edge_iterator& e = it_stack.back();
         const Int to_n = this->to_node(e);
         if (!is_back_edge(to_n) && this->visit_edge(cur, to_n, e)) {
            cur = to_n;
            --this->undiscovered;
            it_stack.push_back(this->edges(to_n));
         } else {
            ++e;
         }
      }
      it_stack.pop_back();
   }

   void propagate()
   {
      for (;; ++it_stack.back()) {
         edge_iterator& e = it_stack.back();
         if (!e.at_end()) {
            const Int to_n = this->to_node(e);
            if (!is_back_edge(to_n) && this->visit_edge(cur, to_n, e)) {
               cur = to_n;
               --this->undiscovered;
               break;
            }
         } else {
            it_stack.pop_back();
            if (it_stack.empty()) {
               cur = -1;
               break;
            }
         }
      }
   }

   bool is_back_edge(Int n) const
   {
      if (!visitor_t::visit_all_edges || TGraph::is_directed) return false;
      const Int s = it_stack.size();
      return s >= 2 && base_t::from_node(it_stack[s-2]) == n;
   }

   it_stack_t it_stack;
   Int cur;
};


class TopologicalSortVisitor
{
public:
   static const bool visit_all_edges=true;

   TopologicalSortVisitor()
      : max_rank(0) {}

   template <typename TGraph>
   TopologicalSortVisitor(const GenericGraph<TGraph>& G)
      : rank(G.top().dim(), 0)
      , max_rank(G.top().nodes()) {}

   template <typename TGraph>
   void clear(const GenericGraph<TGraph>& G)
   {
      std::fill(rank.begin(), rank.end(), 0);
   }

   bool operator()(Int n)
   {
      if (rank[n] == 0) {
         rank[n] = max_rank;
         return true;
      }
      return false;
   }

   bool operator()(Int n_from, Int n_to)
   {
      if (rank[n_to] == 0) {
         rank[n_to] = max_rank;
         return true;
      }
      propagate_back(n_from, n_to);
      return false;
   }

   void propagate_back(Int n_from, Int n_to)
   {
      assign_min(rank[n_from], rank[n_to]-1);
   }

   const std::vector<Int>& get_ranks() const { return rank; }
   std::vector<Int>& get_ranks() { return rank; }

private:
   std::vector<Int> rank;
   Int max_rank;
};


template <typename TGraph, typename = std::enable_if_t<TGraph::is_directed>>
std::pair<std::vector<Int>, Int> topological_sort(const GenericGraph<TGraph>& G)
{
   Int min_rank = G.top().nodes();
   if (min_rank <= 1) return { std::vector<Int>(min_rank, 1), min_rank };

   DFSiterator<TGraph, VisitorTag<TopologicalSortVisitor>> search_it(G.top());
   std::vector<Int>& ranks = search_it.node_visitor_mutable().get_ranks();

   for (auto nodes_it=entire(nodes(G));  !nodes_it.at_end(); ) {
      for (search_it.restart(*nodes_it);  !search_it.at_end();  ++search_it) {
         const Int n_pred = search_it.predecessor();
         if (n_pred >= 0)
            search_it.node_visitor_mutable().propagate_back(n_pred, *search_it);
         else
            assign_min(min_rank, ranks[*search_it]);
      }
      if (search_it.undiscovered_nodes()) {
         do {
            ++nodes_it;
            assert(!nodes_it.at_end());
         } while (ranks[*nodes_it] != 0);
      } else {
         break;
      }
   }

   return { std::move(ranks), min_rank };
}

template <typename TGraph, typename=typename std::enable_if<TGraph::is_directed>::type>
bool is_totally_ordered(const GenericGraph<TGraph>& G)
{
   return topological_sort(G).second <= 1;
}

} }

namespace pm {
   template <typename TGraph, typename... TParams>
   struct check_iterator_feature<polymake::graph::BFSiterator<TGraph, TParams...>, end_sensitive> : std::true_type {};

   template <typename TGraph, typename... TParams>
   struct check_iterator_feature<polymake::graph::DFSiterator<TGraph, TParams...>, end_sensitive> : std::true_type {};
}


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
