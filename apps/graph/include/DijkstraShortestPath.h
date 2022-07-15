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

#include "polymake/type_utils.h"

namespace polymake {
namespace graph {

template <typename TopLayer>
class DijkstraShortestPath
  : public TopLayer {
public:
   using label_t = typename TopLayer::template Label<>;
protected:
   using data_t = typename TopLayer::template Data<DijkstraShortestPath<TopLayer>>;
   using algo_t = typename TopLayer::template Algo<DijkstraShortestPath<TopLayer>>;

   data_t data;
public:
   template <typename... Args, typename = std::enable_if_t<std::is_constructible<data_t, Args...>::value>>
   explicit DijkstraShortestPath(Args&&... args)
      : data(std::forward<Args>(args)...) {}

   class PathReverseIterator {
      friend class DijkstraShortestPath<TopLayer>;

      PathReverseIterator() = delete;
      PathReverseIterator(const PathReverseIterator&) = delete;
      PathReverseIterator& operator= (const PathReverseIterator&) = delete;

   public:
      PathReverseIterator(PathReverseIterator&&) = default;

      bool at_end() const { return !cur_label; }

      Int cur_node() const { return cur_label->node; }

      decltype(auto) cur_weight() const { return cur_label->get_min_cost(); }

      PathReverseIterator& operator++ ()
      {
         cur_label = cur_label->predecessor;
         return *this;
      }

   private:
      explicit PathReverseIterator(const label_t* label)
         : cur_label(label) {}

      const label_t* cur_label;
   };

   // TODO add the multi-start-node variant when desired
   //! perform a shortest path search
   //! @param source source node
   //! @param target predicate telling whether the label is on the target
   //! @param backward_mode if true, search backwards (inverting the edge directions; only defined for directed graphs)
   template <typename Predicate>
   PathReverseIterator solve(Int source, const Predicate& target, bool backward = false)
   {
      algo_t algo(data);
      return PathReverseIterator(algo.solve(source, target, backward));
   }

   //! convenience wrapper for node-to-node search
   PathReverseIterator solve(Int source, Int target, bool backward = false)
   {
      return solve(source, [target](const label_t& label) { return label.node == target; }, backward);
   }
};

} }


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
