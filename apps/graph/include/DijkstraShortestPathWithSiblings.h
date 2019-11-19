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

#ifndef POLYMAKE_GRAPH_DIJKSTRA_SHORTEST_PATH_WITH_SIBLINGS_H
#define POLYMAKE_GRAPH_DIJKSTRA_SHORTEST_PATH_WITH_SIBLINGS_H

#include "polymake/graph/DijkstraShortestPathBase.h"

namespace polymake {
namespace graph {

class DijkstraShortestPathWithSiblings
   : public DijkstraShortestPathBase {
public:
   using base_layer = DijkstraShortestPathBase;

   template <typename FullLabel>
   struct Label : public base_layer::Label<FullLabel> {
      using base_t = base_layer::Label<FullLabel>;
      using base_layer::Label<FullLabel>::Label;

      //! another label on the same node merged into this one and not propagated further
      FullLabel* sibling = nullptr;
      //! label has been updated when absorbing a sibling
      bool is_updated = false;
   };

   struct LabelComparatorWithUpdates : public LabelCostComparator {
      template <typename FullLabel>
      pm::cmp_value operator() (const FullLabel& l1, const FullLabel& l2) const
      {
         const pm::cmp_value cmp_cost = operations::cmp::operator()(l1.get_min_cost(), l2.get_min_cost());
         if (cmp_cost != pm::cmp_eq) return cmp_cost;
         // an updated label should appear first at the heap top
         return l1.is_updated == l2.is_updated ? pm::cmp_eq : l1.is_updated ? pm::cmp_lt : pm::cmp_gt;
      }
   };

   template <typename Top>
   class Data : public base_layer::template Data<Top> {
   public:
      using base_t = typename base_layer::template Data<Top>;
      using typename base_t::label_t;
      using label_comparator_t = typename Top::label_comparator_t;

      using base_layer::template Data<Top>::Data;

      label_t* rotate_siblings(int node)
      {
         label_t* old_first_label = this->labels_on_node[node];
         label_t* first_sibling = old_first_label->sibling;
         old_first_label->sibling = nullptr;
         label_t* next_sibling = first_sibling;
         while (next_sibling->sibling) next_sibling = next_sibling->sibling;
         next_sibling->sibling = old_first_label;
         this->labels_on_node[node] = first_sibling;
         return first_sibling;
      }
   };

   template <typename Top>
   class Algo : public base_layer::template Algo<Top> {
   public:
      using base_t = typename base_layer::template Algo<Top>;
      using typename base_t::graph_t;
      using typename base_t::label_t;
      using typename base_t::data_t;

      void propagate(label_t* const pred_label, const int cur_node, const int edge_id) const
      {
         label_t* cur_label = this->data.labels_on_node[cur_node];
         label_t* new_label = this->top().construct_label(pred_label, cur_node, edge_id);

         if (cur_label) {
            switch (this->top().compare_labels(new_label, cur_label)) {
            case DijkstraLabelCmp::discard_new:
               this->data.reclaim_label(new_label);
               return;
            case DijkstraLabelCmp::discard_old_and_siblings:
               this->top().drop_label(cur_label);
               this->top().erase_label(cur_label);
               break;
            case DijkstraLabelCmp::replace_old:
               this->top().drop_label(cur_label);
               new_label->sibling = cur_label;
               break;
            case DijkstraLabelCmp::discard_old_keep_siblings:
               this->top().drop_label(cur_label);
               new_label->sibling = cur_label->sibling;
               base_t::erase_label(cur_label);
               break;
            case DijkstraLabelCmp::keep_new_update_old:
               this->top().add_sibling(cur_label, new_label, pred_label);
               if (cur_label->heap_pos < 0) {
                  cur_label->is_updated = true;
                  this->data.heap.push(cur_label);
               }
               return;
            case DijkstraLabelCmp::keep_new_as_sibling:
               this->top().add_sibling(cur_label, new_label, pred_label);
               return;
            case DijkstraLabelCmp::propagate_both:
               throw std::runtime_error("DijkstraShortestPathWithSiblings: independent labels not supported yet");
            }
         }
         this->top().push_new_label(new_label, pred_label);
      }

      bool process_popped(label_t* const pred_label, const bool backward) const
      {
         if (pred_label->is_updated) {
            pred_label->is_updated = false;
            if (backward) {
               for (auto edge_it = entire(this->data.G.in_edges(pred_label->node)); !edge_it.at_end(); ++edge_it)
                  propagate_update(pred_label, edge_it.from_node(), *edge_it);
            } else {
               for (auto edge_it = entire(this->data.G.out_edges(pred_label->node)); !edge_it.at_end(); ++edge_it)
                  propagate_update(pred_label, edge_it.to_node(), *edge_it);
            }
            return false;
         }
         return true;
      }

      void add_sibling(label_t* cur_label, label_t* new_label, label_t* pred_label) const
      {
         new_label->set_predecessor(pred_label);
         new_label->refc = 1;
         new_label->sibling = cur_label->sibling;
         cur_label->sibling = new_label;
      }

      void update_siblings(label_t* label) const
      {
         remove_redundant_siblings(label, label, this->top().is_redundant_sibling_after_update());
      }

      void erase_label(label_t* label) const
      {
         do {
            label_t* sibling = label->sibling;
            base_t::erase_label(label);
            label = sibling;
         } while (label);
      }

      template <typename Predicate>
      void remove_redundant_siblings(label_t* const new_label, label_t* prev_sibling, const Predicate& is_redundant) const
      {
         label_t* cur_sibling = prev_sibling->sibling;
         while (cur_sibling) {
            label_t* next_sibling = cur_sibling->sibling;
            if (is_redundant(new_label, cur_sibling)) {
               base_t::erase_label(cur_sibling);
               prev_sibling->sibling = next_sibling;
            } else {
               prev_sibling = cur_sibling;
            }
            cur_sibling = next_sibling;
         }
      }

   protected:
      using base_layer::template Algo<Top>::Algo;

      void propagate_update(label_t* const pred_label, const int cur_node, const int edge_id) const
      {
         label_t* cur_label = this->data.labels_on_node[cur_node];

         // an update of pred_label can have an effect only when there is a descendant label at the current node
         for (label_t* any_label = cur_label; ; any_label = any_label->sibling) {
            if (any_label) {
               if (any_label->predecessor == pred_label) break;
            } else {
               return;
            }
         }

         if (this->top().update_label(pred_label, cur_label, edge_id)) {
            this->top().update_siblings(cur_label);
            if (cur_label->heap_pos < 0) {
               cur_label->is_updated = true;
               this->data.heap.push(cur_label);
            }
         }
      }

   };
};

} }

#endif // POLYMAKE_GRAPH_DIJKSTRA_SHORTEST_PATH_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
