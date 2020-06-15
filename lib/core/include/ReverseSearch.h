/* Copyright (c) 1997-2020
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

/*
   This file contains a templated version of the reverse search algorithm that
   is compatible with the mts framework (https://arxiv.org/abs/1709.07605). The
   central classes to implement are:
   - Node class: A node in the reverse search tree, that is able to give its
     neighbors and predecessor.
   - Logger class: Whenever a node is discovered, this logger is called to
     record the node. While the algorithm might visit a node several times, the
     logger is only called once per node.
   If implemented via this template it is possible to make a binary for
   parallelizing the algorithm via the mts framework.

   The logger can be used to collect the edges of the reverse search tree, or
   even the full graph structure, by means of the two last template parameters.
   See
   - apps/polytope/src/reverse_search_graph.cc for an example collecting the
     full graph
   - apps/fan/src/reverse_search_hyperplaneArr.cc for an example not collecting
     anything.
*/

#ifndef REVERSE_SEARCH_HH
#define REVERSE_SEARCH_HH

namespace polymake {

template<typename Node, typename Logger, bool _make_tree, bool all_edges=false>
class ReverseSearchTemplate {

   private:
      Logger& logger;

   public:
      ReverseSearchTemplate(Logger& l) : logger(l) {}

      long generic_reverse_search(const Node& initial,
            long max_nodes, long max_depth){
         bool overbudget;
         long count = 0;
         Node v(initial);
         logger.log_node(v);
         if(_make_tree || all_edges){
            logger.tree_add_node(v);
         }
         Int Delta = v.get_Delta();
         //log_node(initial);
         Int j=-1, depth=0;
         while((depth != 0) or (j < Delta)){
            overbudget = false;
            while(j < Delta && !overbudget){
               j++;
               if(v.has_jth_child(j)){
                  Node Avj(v.get_jth_child(j));
                  if(all_edges){
                     logger.tree_add_node(Avj);
                     logger.tree_add_edge(v, Avj);
                  }
                  if(Avj.has_predecessor(v)){
                     if(!all_edges && _make_tree){
                        logger.tree_add_node(Avj);
                        logger.tree_add_edge(v, Avj);
                     }
                     Delta = Avj.get_Delta();
                     v = Avj;
                     j = -1;
                     depth++;
                     if ((max_nodes>0 && count >= max_nodes) || 
                           (max_depth>0 && depth==max_depth))
                     {
                        overbudget = true;
                        logger.exit_node(v);
                     } else {
                        logger.log_node(v);
                        count++;
                     }
                  }
               }
            }
            if(depth > 0){
               v = v.get_predecessor(j);
               Delta = v.get_Delta();
               depth--;
            }
         }
         return count;
      }

      void auto_reverse_search(const Node& start){
         generic_reverse_search(start, -1, -1);
         if(_make_tree){
            logger.print_tree();
         }
      }

      /* Hall-Knuth estimate for number of nodes in subtree rooted at
       * initial using one random probe.
       */
      unsigned long long estimate(const Node& initial){
         Node v(initial);
         Int j=-1;
         Int num=0;
         unsigned long long ret = 1;
         unsigned long long d = 1;

         while (1)
         {
            Int Delta = v.get_Delta();
            Int children[Delta];
            num = 0;
            j=-1;
            while (j<Delta){
               j++;
               if(v.has_jth_child(j) && v.get_jth_child(j).has_predecessor(v))
                  children[num++] = j;
            }
            if (num == 0) /* leaf */
               return ret;
            j = rand()%num;
            v = v.get_jth_child(children[j]);
            d *= num;
            ret += d;
         }
         /* unreachable */
         return ret;
      }

};

} // namespace polymake
#endif
