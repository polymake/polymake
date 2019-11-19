/* Copyright (c) 1997-2019
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

#ifndef POLYMAKE_GRAPH_DIJKSTRA_SHORTEST_PATH_WITH_SCALAR_WEIGHTS_H
#define POLYMAKE_GRAPH_DIJKSTRA_SHORTEST_PATH_WITH_SCALAR_WEIGHTS_H

//! @file
//! Implementation of DijkstraShortestPath for a simple case of scalar weights
//! assigned to graph arcs in an EdgeMap.

#include "polymake/graph/DijkstraShortestPathBase.h"

namespace polymake {
namespace graph {

template <typename Dir, typename Weight>
class DijkstraShortestPathWithScalarWeights
   : public DijkstraShortestPathBase {
   using base_layer = DijkstraShortestPathBase;
public:
   template <typename FullLabel = void>
   struct Label : public base_layer::Label<typename mprefer1st<FullLabel, Label<FullLabel>>::type> {
      using base_t = base_layer::Label<typename mprefer1st<FullLabel, Label<FullLabel>>::type>;

      template <typename... CostArgs>
      explicit Label(const int node_, CostArgs&&... cost_args)
         : base_t(node_)
         , cost(std::forward<CostArgs>(cost_args)...) {}

      const Weight& get_min_cost() const { return cost; }

      //! accumulated cost from the source node
      Weight cost;
   };

   using graph_t = Graph<Dir>;
   using edge_weights_map = EdgeMap<Dir, Weight>;
   using label_comparator_t = LabelCostComparator;

   template <typename Top>
   class Data : public base_layer::template Data<Top> {
      using base_t = typename base_layer::template Data<Top>;
   public:
      Data(const graph_t& G_, const edge_weights_map& weights_arg)
         : base_t(G_)
         , edge_weights(weights_arg) {}

      const edge_weights_map& edge_weights;
   };

   template <typename Top>
   class Algo : public base_layer::template Algo<Top> {
   public:
      using base_t = typename base_layer::template Algo<Top>;
      using typename base_t::label_t;
      using typename base_t::data_t;

      using base_layer::template Algo<Top>::Algo;

      label_t* construct_label(const int node) const
      {
         return base_t::construct_label(node, zero_value<Weight>());
      }

      label_t* construct_label(const label_t* pred_label, int cur_node, int edge_id) const
      {
         return base_t::construct_label(cur_node, pred_label->cost + this->data.edge_weights[edge_id]);
      }

      DijkstraLabelCmp compare_labels(const label_t* new_label, const label_t* old_label) const
      {
         const typename data_t::label_comparator_t label_cmp{};
         return label_cmp(*new_label, *old_label) == pm::cmp_lt
                ? DijkstraLabelCmp::discard_old_and_siblings : DijkstraLabelCmp::discard_new;
      }
   };
};

} }

#endif // POLYMAKE_GRAPH_DIJKSTRA_SHORTEST_PATH_WITH_SCALAR_WEIGHTS_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
