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

namespace polymake { namespace graph {

// complete the node set to the lexically minimal max. clique containing it
template <typename Graph> inline
void max_cliques_iterator<Graph>::complete_clique(Set<Int>& set, Set<Int> neighbors)
{
   while (!neighbors.empty()) {
      const Int v = neighbors.front();
      set += v;
      neighbors *= G->adjacent_nodes(v);
   }
}

template <typename Graph>
Set<Int>& max_cliques_iterator<Graph>::lex_min_clique(Set<Int>& set)
{
   complete_clique(set, accumulate(select(rows(adjacency_matrix(*G)), set), operations::mul()));
   return set;
}

template <typename Graph>
Set<Int> max_cliques_iterator<Graph>::lex_min_clique(Int v)
{
   Set<Int> set = scalar2set(v);
   complete_clique(set, G->adjacent_nodes(v));
   return set;
}

template <typename Graph>
void max_cliques_iterator<Graph>::init()
{
   // Different from the cited paper, we start here with all locally lex-min cliques.
   // (They are direct children of the globally lex-min clique anyway.)
   // The reverse search part in the increment operator can be efficiently constrained to the neighborhood
   // of the current clique.

   for (auto n = entire(nodes(*G)); !n.at_end(); ++n) {
      const Int v = n.index();
      if (!n.degree() || n.adjacent_nodes().front()>v)
         Q.push_back(lex_min_clique(v),v);
   }
}

template <typename Graph>
max_cliques_iterator<Graph>& max_cliques_iterator<Graph>::operator++()
{
   Set<Int> K = Q.front().first;
   const Int parent_index = Q.front().second;
   Q.pop_front();

   // 1. Gather candidate expansion nodes from the neighborhood of K
   // Only those with adj(i)*K_{<i} non-empty are interesting;
   // since we are visiting the K's members in increasing order, this is equivalent to (adj(k)_{>k}) \ K

   Set<Int> candidates, neighborhood;
   for (const Int k_el : K) {
      candidates += (G->adjacent_nodes(k_el) >> std::max(k_el, parent_index)) - K;
   }

   // 2. Check each candidate according to lemmas 3. and 4.
   Set<Int> neighbors_of_K_i;
   Set<Int>::const_iterator k_it = K.begin();

   for (const Int i : candidates) {
      while (!k_it.at_end()) {
         if (*k_it > i) break;
         neighbors_of_K_i += G->adjacent_nodes(*k_it) - K;
         ++k_it;
      }
      Set<Int> germ = (K << i) * G->adjacent_nodes(i);
      auto nb_i = G->adjacent_nodes(i).begin();

      // check other candidates whether they could have produced the same clique as this germ
      for (Set<Int>::const_iterator j_it = neighbors_of_K_i.begin(); ; ++j_it) {
         Int j;
         if (j_it.at_end() || (j = *j_it) >= i) {
            // no obstacles: bear a new child clique
            Q[lex_min_clique(germ += i)] = i;
            break;
         }

         if (incl(germ, G->adjacent_nodes(j)) <= 0) {
            // the O(log(degree)) containment queries for each j in adj(i) replaced by parallel increasing of both iterators
            bool j_in_nb_i = false;
            while (!nb_i.at_end()) {
               if (*nb_i == j) {
                  j_in_nb_i = true;
                  break;
               }
               if (*nb_i > j) break;
               ++nb_i;
            }
            if (j_in_nb_i || incl(K << j, G->adjacent_nodes(j)) <= 0) break;
         }
      }
   }

   return *this;
}

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
