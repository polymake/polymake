/* Copyright (c) 1997-2016
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

#ifndef POLYMAKE_GRAPH_BICONNECTED_H
#define POLYMAKE_GRAPH_BICONNECTED_H

#include "polymake/graph/graph_iterators.h"
#include "polymake/IndexedSubset.h"
#include "polymake/IncidenceMatrix.h"

namespace polymake { namespace graph {

/// Implements the Tarjan's algorithm.
/// Delivers one biconnected component less articulation node per iteration.
/// A component is a transient container of node indices.
/// Articulation nodes are to be collected via dedicated access method.
template <typename TGraph>
class biconnected_components_iterator {
protected:
   class NodeVisitor {
      friend class biconnected_components_iterator;
   public:
      static const bool visit_all_edges=true;

      NodeVisitor(const TGraph& G)
         : discovery(G.dim(), -1)
         , low(G.dim(), -1)
         , articulation_nodes(G.dim())
      {
         node_stack.reserve(G.nodes());
      }

      void clear(const TGraph&) = delete;

      // for start nodes
      bool operator() (int n)
      {
         cur_time=-1;
         articulation_node=-1;
         discover(n);
         return true;
      }

      bool operator() (int n_from, int n_to)
      {
         const int d=discovery[n_to];
         if (d>=0) {
            assign_min(low[n_from], d);
            return false;
         }
         discover(n_to);
         return true;
      }

   private:
      bool is_discovered(int n) const
      {
         return discovery[n] >= 0;
      }

      typedef IndexedSubset<const std::vector<int>&, sequence> component_type;

      component_type get_cur_component() const
      {
         return component_type(node_stack, range(discovery[articulation_node], int(node_stack.size()-1)));
      }

      bool is_new_component(int n_from, int n_to)
      {
         if (discovery[n_to] == low[n_to]) {
            // a lone antenna node or an already seen articulation node?
            if (!articulation_nodes.contains(n_to)) {
               articulation_node=n_to;
               articulation_nodes+=n_to;
               return true;
            }
            assert(node_stack.back() == n_to);
            node_stack.pop_back();
            return false;
         }
         if (discovery[n_from] == low[n_to]) {
            // a non-trivial component
            articulation_node=n_from;
            articulation_nodes+=n_from;
            return true;
         }
         assign_min(low[n_from], low[n_to]);
         return false;
      }

      void next_component(int n)
      {
         cur_time=discovery[articulation_node]-(n==articulation_node);
         node_stack.resize(cur_time+1);
         articulation_node=-1;
      }

      void discover(int n)
      {
         discovery[n]=low[n]= ++cur_time;
         node_stack.push_back(n);
      }

      std::vector<int> node_stack, discovery, low;
      Bitset articulation_nodes;
      int articulation_node, cur_time;
   };

   typedef DFSiterator<TGraph, VisitorTag<NodeVisitor>> search_iterator;
   typedef decltype(entire(nodes(std::declval<const TGraph&>()))) nodes_iterator;
public:
   typedef std::forward_iterator_tag iterator_category;
   typedef typename NodeVisitor::component_type value_type;
   typedef value_type reference;
   typedef value_type* pointer;
   typedef ptrdiff_t difference_type;
   typedef biconnected_components_iterator iterator;
   typedef biconnected_components_iterator const_iterator;

   explicit biconnected_components_iterator(const GenericGraph<TGraph>& G)
      : search_it(G)
      , nodes_it(entire(nodes(G)))
   {
      if (!nodes_it.at_end()) {
         search_it.restart(*nodes_it);
         next();
      }
   }

   reference operator* () const
   {
      return search_it.node_visitor().get_cur_component();
   }

   iterator& operator++()
   {
      search_it.node_visitor_mutable().next_component(*search_it);
      ++search_it;
      next();
      return *this;
   }

   iterator operator++(int) { iterator copy(*this); operator++(); return copy; }

   bool at_end() const { return search_it.at_end(); }

   bool operator== (const iterator& other) const { return search_it == other.search_it; }
   bool operator!= (const iterator& other) const { return !operator==(other); }

protected:
   void next()
   {
      for (;; ++search_it) {
         if (search_it.at_end()) {
            if (search_it.undiscovered_nodes()==0)
               break;
            // proceed with the next connected component
            int n;
            do {
               ++nodes_it;
               assert(!nodes_it.at_end());
               n = *nodes_it;
            } while (search_it.node_visitor().is_discovered(n));
            search_it.restart(n);
         }
         if (search_it.node_visitor_mutable().is_new_component(search_it.predecessor(), *search_it))
            break;
      }
   }

   search_iterator search_it;
   nodes_iterator nodes_it;
};

template <typename TGraph> inline
typename std::enable_if<!TGraph::is_directed, IncidenceMatrix<>>::type
biconnected_components(const GenericGraph<TGraph>& G)
{
   RestrictedIncidenceMatrix<only_cols> m(G.top().dim(), rowwise(), biconnected_components_iterator<TGraph>(G));
   return IncidenceMatrix<>(std::move(m));
}

} }

namespace pm {

template <typename TGraph>
struct check_iterator_feature<polymake::graph::biconnected_components_iterator<TGraph>, end_sensitive> : std::true_type {};

}

#endif // POLYMAKE_GRAPH_BICONNECTED_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
