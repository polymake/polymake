/* Copyright (c) 1997-2018
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

#ifndef POLYMAKE_GRAPH_DIJKSTRA_SHORTEST_PATH_BASE_H
#define POLYMAKE_GRAPH_DIJKSTRA_SHORTEST_PATH_BASE_H

#include "polymake/Graph.h"
#include "polymake/Heap.h"
#include "polymake/internal/chunk_allocator.h"
#include <cassert>

namespace polymake {
namespace graph {

//! Result of comparison of two labels
enum class DijkstraLabelCmp {
   //! new label is better in all criteria, old label and all its siblings must be discarded
   discard_old_and_siblings,
   //! old label is better in all criteria, new label must be discarded
   discard_new,
   //! labels are not comparable, both must be propagated further
   propagate_both,
   //! new label is better in the sense of priority queue order,
   //! old label becomes its sibling
   replace_old,
   //! new label is better in the sense of priority queue order,
   //! old label must be discarded but its siblings survive
   discard_old_keep_siblings,
   //! old label is better in the sense of priority queue order,
   //! new label becomes its sibling
   keep_new_as_sibling,
   //! old label is better in the sense of priority queue order but some secondary criteria were improved,
   //! new label becomes its sibling
   keep_new_update_old
};

class DijkstraShortestPathBase {
public:
   template <typename FullLabel>
   struct Label {
      explicit Label(int node_arg)
         : node(node_arg) {}

      void set_predecessor(FullLabel* pred)
      {
         assert(pred && !predecessor);
         predecessor = pred;
         ++pred->refc;
      }

      //! predecessor on the shortest path from the source, nullptr for start node
      FullLabel* predecessor = nullptr;
      //! Graph node index
      int node;
      //! number of successor labels plus 1 for the node or sibling list
      int refc = 0;
      //! position in the priority queue or -1 for already processed labels
      int heap_pos = -1;
   };

   struct LabelCostComparator : public operations::cmp {
      template <typename FullLabel>
      pm::cmp_value operator() (const FullLabel& l1, const FullLabel& l2) const
      {
         return operations::cmp::operator()(l1.get_min_cost(), l2.get_min_cost());
      }
   };

   template <typename Top>
   class Data {
   public:
      using top_t = Top;
      using graph_t = typename Top::graph_t;
      using label_t = typename Top::label_t;
      using labels_on_node_dict = NodeMap<typename graph_t::dir, label_t*>;
      using label_comparator_t = typename Top::label_comparator_t;

      class HeapPolicy {
      public:
         using value_type = label_t*;

         static int position(const label_t* label)
         {
            return label->heap_pos;
         }
         static void update_position(label_t* label, int old_pos, int new_pos)
         {
            label->heap_pos=new_pos;
         }
         const label_t& key(const label_t* label) const
         {
            return *label;
         }
         label_comparator_t key_comparator() const
         {
            return label_comparator_t{};
         }
      };

      using prio_queue_t = Heap<HeapPolicy>;

      explicit Data(const graph_t& G_arg)
         : G(G_arg)
         , labels_on_node(G, nullptr)
         , label_alloc(sizeof(label_t)) {}

      //! remove all labels
      void clear()
      {
         fill_range(entire_range(labels_on_node), nullptr);
         heap.clear();
         label_alloc.clear();
      }

      template <typename... Args, typename = std::enable_if_t<std::is_constructible<label_t, int, Args...>::value>>
      label_t* construct_label(int node, Args&&... args)
      {
         return new(label_alloc.allocate()) label_t(node, std::forward<Args>(args)...);
      }

      void reclaim_label(label_t* label)
      {
         label->~label_t();
         label_alloc.reclaim(label);
      }

      const graph_t& G;
      labels_on_node_dict labels_on_node;
      prio_queue_t heap;
      pm::chunk_allocator label_alloc;
   };

   template <typename Top>
   class Algo {
   public:
      using top_t = Top;
      using graph_t = typename Top::graph_t;
      using label_t = typename Top::label_t;
      using data_t = typename Top::template Data<Top>;
      using algo_top = typename Top::template Algo<Top>;

      explicit Algo(data_t& data_arg)
         : data(data_arg) {}

      const algo_top& top() const { return static_cast<const algo_top&>(*this); }

      template <typename Predicate>
      const label_t* solve(int source, const Predicate& target, bool backward=false) const
      {
         if (backward && graph_t::dir::value)
            throw std::runtime_error("backward search is only defined for directed graphs");
         start_search(source);
         return do_search(target, backward);
      }

      bool process_popped(const label_t* label, bool backward) const
      {
         return true;
      }

      template <typename... Args, typename = std::enable_if_t<std::is_constructible<label_t, int, Args...>::value>>
      label_t* construct_label(Args&&... args) const
      {
         return data.construct_label(std::forward<Args>(args)...);
      }

   protected:
      void start_search(int node) const
      {
         label_t* const label = top().construct_label(node);
         data.labels_on_node[node] = label;
         label->refc = 1;
         data.heap.push(label);
      }

      template <typename Predicate>
      const label_t* do_search(const Predicate& target, bool backward) const
      {
         while (!data.heap.empty()) {
            label_t* const pred_label = data.heap.pop();
            if (top().process_popped(pred_label, backward)) {
               if (target(*pred_label))
                  return pred_label;
            }

            if (backward) {
               for (auto edge_it = entire(data.G.in_edges(pred_label->node)); !edge_it.at_end(); ++edge_it)
                  top().propagate(pred_label, edge_it.from_node(), *edge_it);
            } else {
               for (auto edge_it = entire(data.G.out_edges(pred_label->node)); !edge_it.at_end(); ++edge_it)
                  top().propagate(pred_label, edge_it.to_node(), *edge_it);
            }
         }
         return nullptr;
      }

      void propagate(label_t* const pred_label, const int cur_node, const int cur_edge_id) const
      {
         label_t* cur_label = data.labels_on_node[cur_node];
         if (cur_label && cur_label->heap_pos < 0) {
            // node already settled
            return;
         }
         label_t* new_label = top().construct_label(pred_label, cur_node, cur_edge_id);

         if (cur_label) {
            switch (top().compare_labels(new_label, cur_label)) {
            case DijkstraLabelCmp::discard_new:
               data.reclaim_label(new_label);
               return;
            case DijkstraLabelCmp::discard_old_and_siblings:
               top().drop_label(cur_label);
               top().erase_label(cur_label);
               break;
            default:
               throw std::runtime_error("DijkstraShortestPathBase: unexpected label comparison value");
            }
         }
         top().push_new_label(new_label, pred_label);
      }

      void push_new_label(label_t* new_label, label_t* pred_label) const
      {
         new_label->set_predecessor(pred_label);
         new_label->refc = 1;
         data.labels_on_node[new_label->node] = new_label;
         data.heap.push(new_label);
      }

      void drop_label(label_t* label) const
      {
         if (label->heap_pos >= 0)
            data.heap.erase_at(label->heap_pos);
      }

      void erase_label(label_t* label) const
      {
         if (--label->refc==0) {
            if (label->predecessor) {
               --label->predecessor->refc;
            }
            data.reclaim_label(label);
         }
      }

      data_t& data;
   };
};

} }

#endif // POLYMAKE_GRAPH_DIJKSTRA_SHORTEST_PATH_BASE_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
