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

namespace polymake { namespace fan {

namespace {      

BigObject
stacky_bsd_impl(BigObject StackyFan,
                OptionSet options,
                const Int subdivide_how_many_times)
{
   const bool ignore_top_node = options["ignore_top_node"];
   
   check_stacky_fan(StackyFan);
   const graph::Lattice<graph::lattice::BasicDecoration, graph::lattice::Sequential> HD = StackyFan.give("GENERATING_HASSE_DIAGRAM");
   
   Array<Set<Int>> SD_facets;
   Array<Set<Set<Int>>> SD_labels;
   std::tie(SD_facets, SD_labels) = (1 == subdivide_how_many_times)
      ? topaz::first_barycentric_subdivision (HD, ignore_top_node)
      : topaz::second_barycentric_subdivision(HD, ignore_top_node);

#if POLYMAKE_DEBUG   
   if (get_debug_level() > 1) {
      cerr << "SD_facets:\n" << SD_facets
           << "\nSD_labels:\n";
      for (Int i=0; i<SD_labels.size(); ++i)
         cerr << i << ": " << SD_labels[i] << endl;
      cerr << endl;
   }
#endif
   
   const bool identify_only_on_boundary = options["identify_only_on_boundary"];
   const bool check_id_on_bd = options["check_id_on_bd"];
   
   const Array<Array<Int>> rays_perm_generators = StackyFan.give("GROUP.RAYS_ACTION.GENERATORS");
   const IncidenceMatrix<> VIF = StackyFan.give("GENERATING_MAXIMAL_CONES");

#if POLYMAKE_DEBUG
   if (get_debug_level())
      cerr << "rays_perm_generators:\n" << rays_perm_generators << endl;
#endif
   
   Set<Set<Int>> identified_facets;
   Array<Set<Set<Int>>> identified_labels;
   bool is_any_rep_different_from_original(false);
   identify_facets_and_labels(SD_facets, SD_labels, identify_only_on_boundary, check_id_on_bd, rays_perm_generators, VIF,
                              identified_facets, identified_labels, is_any_rep_different_from_original);

   return
      is_any_rep_different_from_original

      ? BigObject("topaz::SimplicialComplex",
                  "FACETS", identified_facets,
                  "VERTEX_LABELS", make_strings(identified_labels))

      : BigObject("topaz::SimplicialComplex",
                  "FACETS", Set<Set<Int>>(SD_facets),
                  "VERTEX_LABELS", make_strings(SD_labels));
}

}
      
BigObject
stacky_first_bsd(BigObject StackyFan, OptionSet options)
{
   return stacky_bsd_impl(StackyFan, options, 1);
}

BigObject
stacky_second_bsd(BigObject StackyFan, OptionSet options)
{
   return stacky_bsd_impl(StackyFan, options, 2);
}
      
UserFunction4perl("# @category Symmetry"
		  "# Create the stacky first barycentric subdivision of a DisjointStackyFan"
		  "# @param DisjointStackyFan F"
                  "# @option Bool ignore_top_node should the top node of the Hasse diagram be ignored in the subdivision? default 0"
                  "# @option Bool identify_only_on_boundary should identification only be done on the boundary? Default 0"
		  "# @return topaz::SimplicialComplex",
                  &stacky_first_bsd,
		  "stacky_first_bsd(DisjointStackyFan, { ignore_top_node => 0, identify_only_on_boundary => 0, check_id_on_bd => 0 })");


UserFunction4perl("# @category Symmetry"
		  "# Create the stacky second barycentric subdivision of a DisjointStackyFan"
		  "# @param DisjointStackyFan F"
                  "# @option Bool ignore_top_node should the top node of the Hasse diagram be ignored in the subdivision? default 0"
                  "# @option Bool identify_only_on_boundary should identification only be done on the boundary? Default 0"
		  "# @return topaz::SimplicialComplex"
                  "# @example"
                  "# Consider the cone over the standard 2-simplex on which Z_2 acts by interchanging coordinates 0 and 1."
                  "# > $c = new Cone(RAYS=>[[1,0,0],[0,1,0],[0,0,1]], GROUP=>new group::Group(HOMOGENEOUS_COORDINATE_ACTION=>new group::PermutationAction(GENERATORS=>[[1,0,2]])));"
                  "# The stacky fan defined by this cone identifies the rays 0 and 1."
                  "# The property VERTEX_LABELS of the stacky second barycentric subdivision records the orbits of flags of the original fan, "
                  "# while its FACETS record the quotiented simplicial complex built from these flags."
                  "# For a smaller example, let's exclude the top node of the Hasse diagram:"
                  "# > $s2bsd = stacky_second_bsd(stacky_fan($c), ignore_top_node=>1);"
                  "# > print $s2bsd->VERTEX_LABELS;"
                  "# | {{0} {0 2}} {{0} {0 1}} {{0 2} {2}} {{0}} {{0 2}} {{0 1}} {{2}}"
                  "# > print $s2bsd->FACETS;"
                  "# | {0 3}"
                  "# | {0 4}"
                  "# | {1 3}"
                  "# | {1 5}"
                  "# | {2 4}"
                  "# | {2 6}"
                  "# With the face {0 1 2} included, we get:"
                  "# > print stacky_second_bsd(stacky_fan($c))->VERTEX_LABELS;"
                  "# | {{0} {0 1 2} {0 2}} {{0} {0 1} {0 1 2}} {{0 1 2} {0 2} {2}} {{0} {0 2}} {{0} {0 1 2}} {{0 1 2} {0 2}} {{0} {0 1}} {{0 1} {0 1 2}} {{0 2} {2}} {{0 1 2} {2}} {{0}} {{0 2}} {{0 1 2}} {{0 1}} {{2}}",
                  &stacky_second_bsd,
		  "stacky_second_bsd(DisjointStackyFan, { ignore_top_node => 0, identify_only_on_boundary => 0, check_id_on_bd => 0 })");


} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
