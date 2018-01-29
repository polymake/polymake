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

#ifndef POLYMAKE_GRAPH_SHRINKING_LATTICE_H
#define POLYMAKE_GRAPH_SHRINKING_LATTICE_H

#include "polymake/graph/Lattice.h"
#include <algorithm>

namespace polymake { namespace graph {

   /*
    * A lattice which allows for deletion of nodes
    * It is assumed that the top node is never deleted.
    */
   template <typename Decoration, typename SeqType = lattice::Nonsequential>
      class ShrinkingLattice : public Lattice<Decoration, SeqType> {

         protected:
            
            // returns 1 + the maximal rank of a node connected to the top node
            int implicit_top_rank() const {
               return accumulate( attach_member_accessor( 
                        select(this->D,this->in_adjacent_nodes(this->top_node())),
                        ptr2type<Decoration,int,&Decoration::rank>()), operations::max()) + 1; 
            }

         public: 
            // Copy constructor
            ShrinkingLattice() : Lattice<Decoration,SeqType>() {}
            ShrinkingLattice(const Lattice<Decoration, SeqType>& l) : Lattice<Decoration, SeqType>(l) {} 

            void delete_node(int n) {
               this->G.delete_node(n);
            }

            template <typename TSet>
               void delete_nodes(const GenericSet<TSet,int> &nlist) {
                  for(auto n_it : nlist.top()) delete_node(n_it);
               }

            void clear() {
               this->G.clear();
            }

            struct node_exists_pred {
               const Graph<Directed> *G;

               node_exists_pred() : G(0) {}
               node_exists_pred(const Graph<Directed>& G_arg) : G(&G_arg) {}

               typedef int argument_type;
               typedef bool result_type;
               result_type operator() (int n) const { return G->node_exists(n); }
            };

            typedef SelectedSubset< typename SeqType::nodes_of_rank_ref_type, node_exists_pred> nodes_with_gaps;
            typedef SelectedSubset< typename SeqType::nodes_of_rank_type, node_exists_pred> nodes_range_with_gaps;

            const nodes_with_gaps nodes_of_rank(int d) const {
               return nodes_with_gaps(this->rank_map.nodes_of_rank(d),this->G);
            }

            const nodes_range_with_gaps nodes_of_rank_range(int d1, int d2) const {
               return nodes_range_with_gaps(this->rank_map.nodes_of_rank_range(d1,d2),this->G);
            }

            void set_implicit_top_rank() {
               this->D[this->top_node()].rank = implicit_top_rank();
            }

      };

}}

#endif
