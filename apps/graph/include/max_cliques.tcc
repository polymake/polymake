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

namespace polymake { namespace graph {

// complete the node set to the lexically minimal max. clique containing it
template <typename Graph> inline
void max_cliques_iterator<Graph>::complete_clique(Set<int>& set, Set<int> neighbors)
{
   while (!neighbors.empty()) {
      const int v=neighbors.front();
      set += v;
      neighbors *= G->adjacent_nodes(v);
   }
}

template <typename Graph> inline
Set<int>& max_cliques_iterator<Graph>::lex_min_clique(Set<int>& set)
{
   complete_clique(set, accumulate(select(rows(adjacency_matrix(*G)), set), operations::mul()));
   return set;
}

template <typename Graph> inline
Set<int> max_cliques_iterator<Graph>::lex_min_clique(int v)
{
   Set<int> set=scalar2set(v);
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

   for (auto n=entire(nodes(*G)); !n.at_end(); ++n) {
      const int v=n.index();
      if (!n.degree() || n.adjacent_nodes().front()>v)
         Q.push_back(lex_min_clique(v),v);
   }
}

template <typename Graph>
max_cliques_iterator<Graph>& max_cliques_iterator<Graph>::operator++()
{
   Set<int> K=Q.front().first;
   const int parent_index=Q.front().second;
   Q.pop_front();

   // 1. Gather candidate expansion nodes from the neighborhood of K
   // Only those with adj(i)*K_{<i} non-empty are interesting;
   // since we are visiting the K's members in increasing order, this is equivalent to (adj(k)_{>k}) \ K

   Set<int> candidates, neighborhood;
   for (Set<int>::const_iterator k_it=K.begin(); !k_it.at_end(); ++k_it) {
      candidates += (G->adjacent_nodes(*k_it) >> std::max(*k_it,parent_index)) - K;
   }

   // 2. Check each candidate according to lemmas 3. and 4.
   Set<int> neighbors_of_K_i;
   Set<int>::const_iterator k_it=K.begin();

   for (Set<int>::const_iterator c_it=candidates.begin(); !c_it.at_end(); ++c_it) {
      const int i=*c_it;
      while (!k_it.at_end()) {
         if (*k_it>i) break;
         neighbors_of_K_i += G->adjacent_nodes(*k_it) - K;
         ++k_it;
      }
      Set<int> germ=(K<<i)*G->adjacent_nodes(i);
      typename Graph::adjacent_node_list::const_iterator nb_i=G->adjacent_nodes(i).begin();

      // check other candidates whether they could have produced the same clique as this germ
      for (Set<int>::const_iterator j_it=neighbors_of_K_i.begin(); ; ++j_it) {
         int j;
         if (j_it.at_end() || (j=*j_it)>=i) {
            // no obstacles: bear a new child clique
            Q[lex_min_clique(germ+=i)]=i;
            break;
         }

         if (incl(germ,G->adjacent_nodes(j))<=0) {
            // the O(log(degree)) containment queries for each j in adj(i) replaced by parallel increasing of both iterators
            bool j_in_nb_i=false;
            while (!nb_i.at_end()) {
               if (*nb_i==j) {
                  j_in_nb_i=true;
                  break;
               }
               if (*nb_i>j) break;
               ++nb_i;
            }
            if (j_in_nb_i || incl(K<<j,G->adjacent_nodes(j))<=0) break;
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
