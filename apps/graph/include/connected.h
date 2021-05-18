/* Copyright (c) 1997-2021
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
#include "polymake/IncidenceMatrix.h"
#include "polymake/Graph.h"

namespace polymake { namespace graph {

template <typename Iterator, typename TGraph> inline
bool connectivity_via_BFS(const TGraph& G)
{
   if (!G.nodes()) return true;
   for (Iterator it(G, nodes(G).front()); !it.at_end(); ++it) {
      if (it.undiscovered_nodes()==0) return true;
   }
   return false;
}

/// Determine whether an undirected graph is connected.
template <typename TGraph>
typename std::enable_if<!TGraph::is_directed, bool>::type
is_connected(const GenericGraph<TGraph>& G)
{
   return connectivity_via_BFS<BFSiterator<TGraph>>(G.top());
}

/// Determine whether a directed graph is weakly connected.
template <typename TGraph>
typename std::enable_if<TGraph::is_directed, bool>::type
is_weakly_connected(const GenericGraph<TGraph>& G)
{
   return connectivity_via_BFS<BFSiterator<TGraph, TraversalDirectionTag<int_constant<0>>>>(G.top());
}

template <typename TGraph>
class connected_components_iterator
   : protected BFSiterator<TGraph, VisitorTag<NodeVisitor<true>>, TraversalDirectionTag<int_constant<!TGraph::is_directed>>> {
protected:
   typedef BFSiterator<TGraph, VisitorTag<NodeVisitor<true>>, TraversalDirectionTag<int_constant<!TGraph::is_directed>>> base_t;
   Set<Int> component;

   void fill()
   {
      do {
         component += this->queue.front();
         base_t::operator++();
      } while (!base_t::at_end());
   }

public:
   typedef std::forward_iterator_tag iterator_category;
   typedef Set<Int> value_type;
   typedef value_type reference;
   typedef const value_type* pointer;
   typedef ptrdiff_t difference_type;

   typedef connected_components_iterator iterator;
   typedef iterator const_iterator;

   connected_components_iterator() {}

   explicit connected_components_iterator(const GenericGraph<TGraph>& graph_arg)
      : base_t(graph_arg)
   {
      rewind();
   }

   reference operator* () const { return component; }
   pointer operator-> () const { return &component; }

   iterator& operator++ ()
   {
      component.clear();
      if (this->undiscovered_nodes() != 0) {
         this->process(this->visitor.get_visited_nodes().front());
         fill();
      }
      return *this;
   }
   const iterator operator++ (int) { iterator copy(*this);  operator++();  return copy; }

   bool operator== (const iterator& it) const { return component==it.component; }
   bool operator!= (const iterator& it) const { return !operator==(it); }

   bool at_end() const { return component.empty(); }

   void rewind()
   {
      if (this->graph->nodes()) {
         this->reset(nodes(*this->graph).front());
         component.clear();
         fill();
      }
   }
};

/// Compute the connected components of an undirected graph
template <typename TGraph> inline
typename std::enable_if<!TGraph::is_directed, IncidenceMatrix<>>::type
connected_components(const GenericGraph<TGraph>& G)
{
   RestrictedIncidenceMatrix<only_cols> m(G.top().dim(), rowwise(), connected_components_iterator<TGraph>(G));
   return IncidenceMatrix<>(std::move(m));
}

/// Compute the weakly connected components of a directed graph
template <typename TGraph> inline
typename std::enable_if<TGraph::is_directed, IncidenceMatrix<>>::type
weakly_connected_components(const GenericGraph<TGraph>& G)
{
   RestrictedIncidenceMatrix<only_cols> m(G.top().dim(), rowwise(), connected_components_iterator<TGraph>(G));
   return IncidenceMatrix<>(std::move(m));
}

/// Construct a connectivity graph of components of another graph
template <typename TGraph>
Graph<typename TGraph::dir>
component_connectivity(const GenericGraph<TGraph>& G, const IncidenceMatrix<>& C)
{
   Graph<typename TGraph::dir> result(C.rows());
   for (auto e=entire(edges(G)); !e.at_end(); ++e) {
      for (auto comp1=entire(C.col(e.from_node())); !comp1.at_end(); ++comp1) {
         for (auto comp2=entire(C.col(e.to_node())); !comp2.at_end(); ++comp2) {
            if (*comp1 != *comp2)
               result.add_edge(*comp1, *comp2);
         }
      }
   }
   return result;
}

} }

namespace pm {

template <typename TGraph>
struct check_iterator_feature<polymake::graph::connected_components_iterator<TGraph>, end_sensitive> : std::true_type {};

template <typename TGraph>
struct check_iterator_feature<polymake::graph::connected_components_iterator<TGraph>, rewindable> : std::true_type {};

template <typename GraphRef>
class generic_of_GraphComponents<GraphRef, polymake::graph::connected_components_iterator>
   : public GenericSet< GraphComponents<GraphRef, polymake::graph::connected_components_iterator>, Set<Int>, operations::cmp > {};
}


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
