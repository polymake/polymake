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

#include "polymake/client.h"
#include "polymake/fan/stacky_fan.h"
#include "polymake/Graph.h"

namespace polymake { namespace fan {

using graph::lattice::BasicDecoration;
using graph::lattice::Sequential;
      
BigObject
fundamental_domain(BigObject C)
{
   const graph::Lattice<BasicDecoration, Sequential> HD = C.give("GENERATING_HASSE_DIAGRAM");
   const Array<Array<Int>> rays_perm_generators = C.give("GROUP.RAYS_ACTION.GENERATORS");

   Array<Set<Int>> SD_facets;
   Array<Set<Set<Int>>> SD_labels;
   std::tie(SD_facets, SD_labels) = topaz::first_barycentric_subdivision(HD);

   BigObject SC("topaz::SimplicialComplex",
                "FACETS", SD_facets);
   const Graph<> dual_graph = SC.give("DUAL_GRAPH.ADJACENCY");
   
   // Each vertex of the barycentric subdivision is represented as a Set<Set<Int>>
   // The rays_perm_generators permute the indices of the inner Set<Int>
   // So we first have to figure out the canonical representatives of those inner Set<Int>s
   bool is_any_rep_different_from_original(false);
   const Map<Set<Int>, Set<Int>> lex_min_rep_of = lex_min_reps_of_inners(rays_perm_generators, SD_labels, is_any_rep_different_from_original);
   if (get_debug_level())
      cerr << "lex_min_rep_of: " << lex_min_rep_of << endl;

   if (!is_any_rep_different_from_original)
      return BigObject("topaz::SimplicialComplex",
                       "FACETS", SD_facets,
                       "VERTEX_LABELS", make_strings(SD_labels));

   Int max_label_rep_index(0);
   Map<Set<Set<Int>>, Int> index_of_label_rep;
   Array<Int> rep_index_of_SD_label(SD_labels.size());
   const bool
      check_id_on_bd(false),
      identify_only_on_boundary(false),
      conserve_label_cardinality(true);
   IncidenceMatrix<> VIF;
   Map<Set<Int>,Set<Set<Int>>> duplicate_labels_of;
   identify_and_index_labels(SD_labels, lex_min_rep_of, VIF, check_id_on_bd, identify_only_on_boundary, conserve_label_cardinality, max_label_rep_index, index_of_label_rep, rep_index_of_SD_label, duplicate_labels_of);

   if (get_debug_level())
      cerr << "index_of_label_rep:\n" << index_of_label_rep << endl
           << "SD_labels: " << SD_labels << endl
           << "rep_index_of_SD_label:\n" << rep_index_of_SD_label << endl
           << "duplicate_labels_of:\n" << duplicate_labels_of << endl << endl;
   
   Set<Set<Int>> identified_facets, facet_reps;
   Set<Int> seen_indices;
   std::list<Int> queue {0};
   while (queue.size()) {
      const Int current_index(queue.front()); queue.pop_front();
      if (get_debug_level()>1)
         cerr << "\nqueue now " << queue << ", current_index: " << current_index << ": " << SD_facets[current_index] << endl;
      seen_indices += current_index;
      Set<Int> image;
      for (const auto& i: SD_facets[current_index])
         image += rep_index_of_SD_label[i];
      if (get_debug_level()>1)
         cerr << "image: " << image << endl;
      
      if (identified_facets.collect(image))
         continue;
      facet_reps += SD_facets[current_index];
      if (get_debug_level()>1)
         cerr << "new. facet_reps now " << facet_reps << endl;
      
      for (auto nb_iter = entire(dual_graph.adjacent_nodes(current_index)); !nb_iter.at_end(); ++nb_iter)
         if (!seen_indices.contains(*nb_iter)) {
            queue.push_back(*nb_iter);
            if (get_debug_level()>1)
               cerr << "added neighbor " << *nb_iter << ": " << SD_facets[*nb_iter] << endl;
         }
   }

   BigObject FD("topaz::SimplicialComplex",
                "FACETS", facet_reps,
                "VERTEX_LABELS", make_strings(SD_labels));
   FD.attach("DUPLICATE_LABELS_OF") << duplicate_labels_of;
   return FD;
}
                          
      
UserFunction4perl("# @category Symmetry"
		  "# Find a fundamental domain with connected DUAL_GRAPH for a cone modulo the action of a symmetry group"
                  "# by performing a breadth-first search on the first barycentric subdivision."
                  "# Some elements in the labels of a vertex of the first barycentric subdivision may get identified;"
                  "# such instances are recorded in an attachment DUPLICATE_LABELS_OF of type Map<Set<Int>, Set<Set<Int>>>."
                  "# @param DisjointStackyFan F"
                  "# @return topaz::SimplicialComplex",
                  &fundamental_domain,
                  "fundamental_domain(DisjointStackyFan)");


} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
    
