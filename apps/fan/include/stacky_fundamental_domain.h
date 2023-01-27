/* Copyright (c) 1997-2023
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

#ifndef __FAN__STACKY_FUNDAMENTAL_DOMAIN_H
#define __FAN__STACKY_FUNDAMENTAL_DOMAIN_H

#include "polymake/fan/stacky_fan.h"

namespace polymake { namespace fan {

template<typename InnerLabel>   
void
fundamental_domain_in_first_bc(const Array<Set<Int>>& SD_facets,
                               const Array<Set<InnerLabel>>& SD_labels,
                               const Array<Array<Int>>& rays_perm_generators,
                               Array<Set<Int>>& reordered_facet_reps,
                               Array<Set<InnerLabel>>& reordered_label_reps)
{
#if MY_POLYMAKE_DEBUG
   const Int debug_level(3); //get_debug_level());
#endif
   
   BigObject SC("topaz::SimplicialComplex",
                "FACETS", SD_facets);
   const Graph<> dual_graph = SC.give("DUAL_GRAPH.ADJACENCY");

   Array<Set<Int>> inner_label_orbits;
   Array<Array<Int>> generators_on_inner_labels;
   Array<Set<Int>> indexed_SD_facets;
   Array<Set<Int>> indexed_SD_labels;
   Array<Set<Int>> indexed_label_orbits;
   Map<Set<Int>, Int> index_of_indexed_label_orbit;
   Array<Array<Int>> generators_on_indexed_label_orbits;
   index_label_orbits(SD_facets, SD_labels, rays_perm_generators,
                      inner_label_orbits, generators_on_inner_labels, indexed_SD_facets, indexed_SD_labels, indexed_label_orbits, index_of_indexed_label_orbit, generators_on_indexed_label_orbits);
   
   const group::PermlibGroup indexed_label_g(generators_on_indexed_label_orbits);
   Set<Int> seen_indices {0};
   std::list<Int> queue {0};
   std::vector<Int> accepted_indices;
   Set<Set<Int>> facet_reps;

   while (queue.size()) {
      const Int current_index(queue.front()); queue.pop_front();

#if MY_POLYMAKE_DEBUG
      if (debug_level>2)
         cerr << "\nqueue now " << queue << ", current_index: " << current_index << ": " << indexed_SD_facets[current_index] << endl;
#endif
      const Set<Int> rep = indexed_label_g.lex_min_representative(indexed_SD_facets[current_index]);

#if MY_POLYMAKE_DEBUG
      if (debug_level>2)
         cerr << "rep of " << indexed_SD_facets[current_index] << ": "
              << "(orbit " << indexed_label_g.orbit(indexed_SD_facets[current_index]) << "): "
              << rep << endl;
#endif
      
      if (facet_reps.collect(rep)) 
         continue;

      accepted_indices.push_back(current_index);
      
      for (auto nb_iter = entire(dual_graph.adjacent_nodes(current_index)); !nb_iter.at_end(); ++nb_iter)
         if (!seen_indices.contains(*nb_iter)) {
            queue.push_back(*nb_iter);
            seen_indices += *nb_iter;

#if MY_POLYMAKE_DEBUG
            if (debug_level>2)
               cerr << "added neighbor " << *nb_iter << ": " << indexed_SD_facets[*nb_iter] << endl;
#endif
         }
   }

   // facet_reps now contains one representative of each orbit of simplices in the barycentric subdivision, expressed in the original vertex indices
   // we now reorder these indices and store the appropriate original vertex labels, without identifications
   
#if MY_POLYMAKE_DEBUG
   if (debug_level)
      cerr << "\nfundamental_domain_impl, before index reordering:\n"
           << "accepted_indices(" << accepted_indices.size() << "):\n" << accepted_indices << endl;
#endif

   if (convert_to<Int>(accepted_indices.size()) == indexed_SD_facets.size()) {
#if MY_POLYMAKE_DEBUG
      if (debug_level)
         cerr << "\ninput is a fundamental domain." << endl;
#endif

      reordered_facet_reps = SD_facets;
      reordered_label_reps = SD_labels;
      return;
   }

   Set<Int> used_inner_label_indices;
   for (auto F_it = entire(select(indexed_SD_facets, accepted_indices)); !F_it.at_end(); ++F_it)
      used_inner_label_indices += *F_it;

   Array<Int> reordered_index_of_inner_label;   
   reordered_facet_reps.resize(accepted_indices.size());
   
   reorder_indices(entire(select(indexed_SD_facets, accepted_indices)), indexed_label_orbits, inner_label_orbits, used_inner_label_indices, generators_on_indexed_label_orbits,
                   entire(reordered_facet_reps), reordered_label_reps, reordered_index_of_inner_label);

   
#if MY_POLYMAKE_DEBUG
   if (debug_level) {
      cerr << "\nfundamental_domain_impl, after index reordering:\n"
           << "reordered_facet_reps(" << reordered_facet_reps.size() << "):\n" << reordered_facet_reps << "\n"
           << "reordered_label_reps(" << reordered_label_reps.size() << "):\n";
      for (Int i=0; i<reordered_label_reps.size(); ++i)
         cerr << i << ": " << reordered_label_reps[i] << "\n";
      cerr << endl;
   }
#endif
}




}}

#endif // __FAN__STACKY_FUNDAMENTAL_DOMAIN_H


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
