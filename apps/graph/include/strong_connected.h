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

#include "polymake/graph/graph_iterators.h"
#include "polymake/IndexedSubset.h"
#include "polymake/IncidenceMatrix.h"

namespace polymake { namespace graph {

/// Implements the Tarjan's algorithm.
/// Delivers one strong component per iteration.
/// A component is a transient container of node indices.
template <typename TGraph>
class strong_components_iterator {
protected:
   class NodeVisitor {
      friend class strong_components_iterator;
   public:
      static const bool visit_all_edges=true;

      NodeVisitor(const TGraph& G)
         : discovery(G.dim(), -1)
         , low(G.dim(), -1)
         , max_time(0)
      {
         node_stack.reserve(G.nodes());
      }

      void clear(const TGraph&) = delete;

      // for start nodes
      bool operator() (Int n)
      {
         start_time=max_time;
         cur_time=max_time-1;
         discover(n);
         return true;
      }

      bool operator() (Int n_from, Int n_to)
      {
         const Int d = discovery[n_to];
         if (d >= 0) {
            if (d >= start_time) {
               // item is on the stack
               assign_min(low[n_from], d);
            }
            return false;
         }
         discover(n_to);
         return true;
      }

   private:
      bool is_discovered(Int n) const
      {
         return discovery[n] >= 0;
      }

      using component_type = IndexedSubset<const std::vector<Int>&, const sequence>;

      component_type get_cur_component(Int n) const
      {
         return component_type(node_stack, range(discovery[n] - start_time, Int(node_stack.size()-1)));
      }

      bool is_new_component(Int n) const
      {
         return discovery[n] == low[n];
      }

      void set_same_component(Int n_from, Int n_to)
      {
         assign_min(low[n_from], low[n_to]);
      }

      void next_component(Int n)
      {
         assign_max(max_time, cur_time+1);
         cur_time = discovery[n];
         node_stack.resize((cur_time--) - start_time);
      }

      void discover(Int n)
      {
         discovery[n] = low[n] = ++cur_time;
         node_stack.push_back(n);
      }

      std::vector<Int> node_stack, discovery, low;
      Int cur_time, start_time, max_time;
   };

   typedef DFSiterator<TGraph, VisitorTag<NodeVisitor>> search_iterator;
   typedef decltype(entire(nodes(std::declval<const TGraph&>()))) nodes_iterator;
public:
   typedef std::forward_iterator_tag iterator_category;
   typedef typename NodeVisitor::component_type value_type;
   typedef value_type reference;
   typedef value_type* pointer;
   typedef ptrdiff_t difference_type;
   typedef strong_components_iterator iterator;
   typedef strong_components_iterator const_iterator;

   explicit strong_components_iterator(const GenericGraph<TGraph>& G)
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
      return search_it.node_visitor().get_cur_component(*search_it);
   }

   iterator& operator++()
   {
      search_it.node_visitor_mutable().next_component(*search_it);
      if ((++search_it).at_end()) {
         if (search_it.undiscovered_nodes()==0)
            return *this;
         // find a new root
         Int root;
         do {
            ++nodes_it;
            assert(!nodes_it.at_end());
            root = *nodes_it;
         } while (search_it.node_visitor().is_discovered(root));
         search_it.restart(root);
      }
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
      do {
         if (search_it.node_visitor().is_new_component(*search_it))
            break;
         else
            search_it.node_visitor_mutable().set_same_component(search_it.predecessor(), *search_it);
      } while (!(++search_it).at_end());
   }

   search_iterator search_it;
   nodes_iterator nodes_it;
};

/// Compute the strong components of a directed graph.
template <typename TGraph> inline
typename std::enable_if<TGraph::is_directed, IncidenceMatrix<>>::type
strong_components(const GenericGraph<TGraph>& G)
{
   RestrictedIncidenceMatrix<only_cols> m(G.top().dim(), rowwise(), strong_components_iterator<TGraph>(G));
   return IncidenceMatrix<>(std::move(m));
}

/// Determine whether a directed graph is strongly connected
template <typename TGraph> inline
typename std::enable_if<TGraph::is_directed, bool>::type
is_strongly_connected(const GenericGraph<TGraph>& G)
{
   strong_components_iterator<TGraph> c(G);
   return c.at_end() || (*c).size()==G.top().nodes();
}

} }

namespace pm {

template <typename TGraph>
struct check_iterator_feature<polymake::graph::strong_components_iterator<TGraph>, end_sensitive> : std::true_type {};

}


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
