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

#include "polymake/Graph.h"
#include "polymake/PowerSet.h"
#include "polymake/Map.h"

/** @file max_cliques.h
 *  Algorithm for generating all maximal cliques of an undirected graph in lexicographical order.
 *  Based on the ideas described in:
 *
 *     Kazuhisa Makino and Takeaki Uno,
 *     New Algorithms for Enumerating All Maximal Cliques,
 *       in:
 *     T. Hagerup and J. Katajainen (Eds.),
 *     SWAT 2004: 9th Scandinavian Workshop On Algorithm Theory,
 *     Springer LNCS 3111, pp. 260-272, 2004
 */

namespace polymake { namespace graph {

template <typename Graph>
class max_cliques_iterator {
protected:
   const Graph *G;
   Map<Set<Int>, Int> Q;

   void complete_clique(Set<Int>& set, Set<Int> neighbors);
   Set<Int>& lex_min_clique(Set<Int>& set);
   Set<Int> lex_min_clique(Int v);
   void init();
public:
   typedef Set<Int> value_type;
   typedef const value_type& reference;
   typedef const value_type* pointer;
   typedef std::forward_iterator_tag iterator_category;
   typedef ptrdiff_t difference_type;
   typedef max_cliques_iterator iterator;
   typedef max_cliques_iterator const_iterator;

   max_cliques_iterator() : G(0) {}
   max_cliques_iterator(const Graph& G_arg) : G(&G_arg) { init(); }

   reference operator* () const { return Q.front().first; }
   pointer operator-> () const { return &(operator*()); }

   iterator& operator++();
   const iterator operator++ (int) { iterator copy(*this); operator++(); return *this; }

   bool at_end() const { return Q.empty(); }
   bool operator== (const iterator& it) const
   {
      if (at_end()) return it.at_end();
      return !it.at_end() && *(*this)==*it;
   }
   bool operator!= (const iterator& it) const { return !operator==(it); }

   void rewind() { Q.clear(); init(); }
};

} }

namespace pm {

template <typename GraphRef>
class generic_of_GraphComponents<GraphRef, polymake::graph::max_cliques_iterator>
   : public GenericSet< GraphComponents<GraphRef,polymake::graph::max_cliques_iterator>, Set<Int>, operations::cmp > {};

}

namespace polymake { namespace graph {

template <typename TGraph>
auto max_cliques(const GenericGraph<TGraph, Undirected>& G)
{
   return pm::GraphComponents<const TGraph&, max_cliques_iterator>(G.top());
}

template <typename TGraph>
auto max_independent_sets(const GenericGraph<TGraph, Undirected>& G)
{
   return pm::GraphComponents<Graph<>&&, max_cliques_iterator>(~G);
}

} }

namespace pm {
template <typename Graph>
struct check_iterator_feature<polymake::graph::max_cliques_iterator<Graph>, end_sensitive> : std::true_type {};
template <typename Graph>
struct check_iterator_feature<polymake::graph::max_cliques_iterator<Graph>, rewindable> : std::true_type {};

}

#include "polymake/graph/max_cliques.tcc"


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
