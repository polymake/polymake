/* Copyright (c) 1997-2022
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
#include "polymake/fan/stacky_fundamental_domain.h"
#include "polymake/topaz/hasse_diagram.h"
#include "polymake/graph/LatticePermutation.h"

namespace polymake { namespace fan {

   using graph::Lattice;
   using graph::lattice::BasicDecoration;
   using graph::lattice::Sequential;
   using graph::lattice::Nonsequential;


template<typename Scalar>      
Set<Int>
orbit_rep(const Matrix<Scalar>& subdiv_rays,
          const hash_set<Array<Int>>& entire_group,
          const Map<Vector<Scalar>, Int>& index_of_ray_orbit_member,
          const Set<Int>& face)
{
   Set<Set<Int>> the_orbit;
   Set<Int> image;
   for (const Array<Int>& homog_coo_perm: entire_group) {
      pm::operations::group::action<Vector<Scalar>, pm::operations::group::on_container, Array<Int>> this_action(homog_coo_perm);
      image.clear();
      for (const Int i: face)
         image += index_of_ray_orbit_member[this_action(subdiv_rays[i])];
      the_orbit += image;
   }
   return the_orbit.front();
}

template<typename Scalar>
std::vector<Int>
find_facet_rep_indices(const Array<Set<Int>>& SD_facets,
                       const Matrix<Scalar>& subdiv_rays,
                       const Map<Vector<Scalar>, Int>& index_of_ray_orbit_member,
                       const hash_set<Array<Int>>& entire_group,
                       const Graph<>& dual_graph,
                       const Int verbosity)
{
   Set<Int> seen_indices {0};
   std::list<Int> queue {0};
   std::vector<Int> accepted_indices;
   Set<Set<Int>> facet_reps;
   
   while (queue.size()) {
      if (facet_reps.size() != convert_to<Int>(accepted_indices.size())) {
         cerr << "problem: facet_reps (" << facet_reps.size() << "):\n" << facet_reps << endl
              << "accepted_indices (" << accepted_indices.size() << "):\n" << accepted_indices << endl;
         throw std::runtime_error("stop");
      }
      const Int current_index(queue.front()); queue.pop_front();
      const Set<Int> rep = orbit_rep(subdiv_rays, entire_group, index_of_ray_orbit_member, SD_facets[current_index]);

      if (verbosity>2)
         cerr << "\nqueue now " << queue << ", current_index: " << current_index << ": " << SD_facets[current_index]
              << " -> " << rep << endl;

      if (facet_reps.collect(rep)) 
         continue;
      
      accepted_indices.push_back(current_index);
      if (verbosity > 2)
         cerr << "accepted " << current_index << " = " << SD_facets[current_index] << endl;
      
      for (auto nb_iter = entire(dual_graph.adjacent_nodes(current_index)); !nb_iter.at_end(); ++nb_iter)
         if (!seen_indices.contains(*nb_iter)) {
            queue.push_back(*nb_iter);
            seen_indices += *nb_iter;

            if (verbosity > 2)
               cerr << "added neighbor " << *nb_iter << ": " << SD_facets[*nb_iter] << endl;
         }
   }
   if (verbosity > 2)
      cerr << "done with queue" << endl << endl;
   
   return accepted_indices;
}

template<typename LabelType, typename Scalar>      
struct FacetsLabelsCoordinates {
   FacetsLabelsReordering<LabelType> flr;
   Matrix<Scalar> coordinates;
   Matrix<Scalar> used_original_coordinates;
};

template<typename Scalar, typename FacetType, typename LabelType>      
FacetsLabelsCoordinates<LabelType, Scalar>
squeeze_data(const FacetType& facets,
             const LabelType& labels,
             const Matrix<Scalar>& coordinates,
             const Int verbosity)
{
   FacetsLabelsCoordinates<LabelType, Scalar> squeezed_flc;
   squeezed_flc.flr = squeeze_facets_and_labels(facets, labels, verbosity);
   squeezed_flc.coordinates = coordinates.minor(squeezed_flc.flr.reordering, All);
   
   Set<Int> used_original_labels;
   for (const auto& ssi: squeezed_flc.flr.labels)
      used_original_labels += ssi.front();
   
   squeezed_flc.used_original_coordinates = coordinates.minor(used_original_labels, All);
   return squeezed_flc;
}

template<typename Scalar>
void
paranoid_consistency_test(const Array<Set<Int>>& SD_facets,
                          const Matrix<Scalar>& subdiv_rays,
                          const Map<Vector<Scalar>, Int>& index_of_ray_orbit_member,
                          const hash_set<Array<Int>>& entire_group,
                          const std::vector<Int>& accepted_indices)
{
   cerr << "paranoid_consistency_test" << endl;
   Set<Set<Int>> accepted_reps;
   for (const Int i: accepted_indices)
      accepted_reps += orbit_rep(subdiv_rays, entire_group, index_of_ray_orbit_member, SD_facets[i]);
   if (accepted_reps.size() != convert_to<Int>(accepted_indices.size())) {
      cerr << "problem: accepted_indices(" << accepted_indices.size() << "):\n" << accepted_indices << endl
           << "accepted_reps(" << accepted_reps.size() << "):\n" << accepted_reps << endl << endl;
      throw std::runtime_error("stop");
   }
   cerr << "Each rep represents one orbit" << endl;

   Set<Set<Int>> all_reps;
   for (const auto& f: SD_facets)
      all_reps += orbit_rep(subdiv_rays, entire_group, index_of_ray_orbit_member, f);
   if (all_reps != accepted_reps) {
      cerr << "problem: all_reps(" << all_reps.size() << "):\n" << all_reps << endl 
           << "accepted_reps(" << accepted_reps.size() << "):\n" << accepted_reps << endl << endl;
      throw std::runtime_error("stop");
   }
   cerr << "Each original simplex has a representative" << endl;

   Map<Set<Int>,Int> index_of_facet_image;
   for (auto fit = entire<indexed>(SD_facets); !fit.at_end(); ++fit) 
      index_of_facet_image[*fit] = fit.index();


   Map<Vector<Scalar>, Int> index_of_ray;
   for (auto rit = entire<indexed>(rows(subdiv_rays)); !rit.at_end(); ++rit)
      index_of_ray[Vector<Scalar>(*rit)] = rit.index();

   Set<Int> seen_indices;
   std::vector<Set<Int>> image_indices_store;
   for (Int i: accepted_indices) {
      Set<Int> image_indices;
      Set<Set<Int>> image_simplices;
      Set<Int> image_simplex;
      for (const auto& g: entire_group) {
         pm::operations::group::action<Vector<Scalar>, pm::operations::group::on_container, Array<Int>> this_action(g);
         image_simplex.clear();
         for (const Int j: SD_facets[i]) {
            const Vector<Scalar> ray_image(this_action(subdiv_rays[j]));
            if (!index_of_ray.contains(ray_image)) {
               const Int next_index(index_of_ray.size());
               index_of_ray[ray_image] = next_index;
            }
            image_simplex += index_of_ray[ray_image];
         }
         if (!index_of_facet_image.contains(image_simplex)) {
            const Int next_index(index_of_facet_image.size());
            index_of_facet_image[image_simplex] = next_index;
         }
         image_indices += index_of_facet_image[image_simplex];
         image_simplices += image_simplex;
      }
      seen_indices += image_indices;
      image_indices_store.push_back(image_indices);
   }
   
   if (incl(Set<Int>(sequence(0, SD_facets.size())), seen_indices) > 0)  {
      cerr << "problem: have not exhausted all simplices:\n" << seen_indices << endl;
      throw std::runtime_error("stop");
   }
   
   cerr << "reps represent all simplices." << endl;

   for (std::size_t i=0; i<image_indices_store.size() - 1; ++i)
      for (std::size_t j=i+1; j<image_indices_store.size(); ++j)
         if ((image_indices_store[i] * image_indices_store[j]).size()) {
            cerr << "...but some simplices are not uniquely represented:" << endl
                 << i << ": " << image_indices_store[i] << endl
                 << j << ": " << image_indices_store[j] << endl << endl
                 << "store:\n";
            for (std::size_t k=0; k <= std::max(i,j); ++k)
               cerr << k << ": " << image_indices_store[k] << endl;
            throw std::runtime_error("stop");
         }
   
   cerr << "test passed. size of fundamental domain: " << accepted_indices.size() << endl << endl;
}

template<typename Scalar>      
BigObject
stacky_fundamental_domain(BigObject StackyFan, OptionSet options)
{
   const Int verbosity = options["verbosity"];
   if (verbosity)
      cerr << "\n*** in stacky_fundamental_domain ***" << endl;
   
   const Matrix<Scalar> generating_rays = StackyFan.give("GENERATING_RAYS");
   if (verbosity > 2) {
      cerr << "generating_rays:\n";
      for (Int i=0; i<generating_rays.rows(); ++i)
         cerr << i << ": " << generating_rays[i] << endl;
      cerr << endl << endl;
   }

   const bool has_trivial_group(!StackyFan.exists("GROUP"));
   if (has_trivial_group && verbosity > 2)
      cerr << "trivial automorphism group" << endl << endl;
   
   const Lattice<BasicDecoration, Sequential> HD = StackyFan.give("GENERATING_HASSE_DIAGRAM");
   
   const auto facets_and_labels = topaz::first_barycentric_subdivision(HD);
   const auto& SD_facets(facets_and_labels.facets);
   const auto& SD_labels(facets_and_labels.labels);
   
   if (verbosity > 2) {
      cerr << "SD_labels (" << SD_labels.size() << "):\n";
      for (Int i=0; i<SD_labels.size(); ++i)
         cerr << i << ": " << SD_labels[i] << endl;
      cerr << endl << endl
           << "SD_facets (" << SD_facets.size() << "):\n" << SD_facets << endl << endl;
   }
      
   Matrix<Scalar> subdiv_rays(SD_labels.size(), generating_rays.cols());
   for (Int i=0; i<SD_labels.size(); ++i) 
      subdiv_rays[i] = accumulate(rows(generating_rays.minor(SD_labels[i].front(), All)), operations::add());
   if (verbosity > 2) {
      cerr << "subdiv_rays:\n";
      for (Int i=0; i<subdiv_rays.rows(); ++i)
         cerr << i << ": " << subdiv_rays[i] << endl;
      cerr << endl << endl;
   }

   if (has_trivial_group) {
      BigObject fd("topaz::GeometricSimplicialComplex", mlist<Scalar>(),
                   "COORDINATES", subdiv_rays,
                   "FACETS", SD_facets);
      fd.attach("IS_FUNDAMENTAL_DOMAIN") << bool(true);
      fd.attach("VERTEX_LABELS_AS_SETS") << SD_labels;
      fd.attach("USED_ORIGINAL_COORDINATES") << subdiv_rays;

      return fd;
   }
   
   const Array<Array<Int>> homog_action_generators = StackyFan.give("GROUP.HOMOGENEOUS_COORDINATE_ACTION.GENERATORS");
   if (verbosity > 3)
      cerr << "action generators:\n" << homog_action_generators << endl;
   
   Int max_index(0);
   Map<Vector<Scalar>, Int> index_of_ray_orbit_member;
   for (const auto& v: rows(subdiv_rays)) {
      const auto the_orbit(group::unordered_orbit<pm::operations::group::on_container,
                                                  Array<Int>, Vector<Scalar>>(homog_action_generators, Vector<Scalar>(v)));

      // if one element of the orbit has already been indexed, go to the next orbit
      if (index_of_ray_orbit_member.exists(*(the_orbit.begin())))
         continue; 
      for (const auto& member: the_orbit)
         if (!index_of_ray_orbit_member.exists(member)) 
            index_of_ray_orbit_member[member] = max_index++;
   }
   
   if (verbosity > 2)
      cerr << "\nindex_of_ray_orbit_member (" << index_of_ray_orbit_member.size() << "):\n" << index_of_ray_orbit_member << endl << endl;

   const hash_set<Array<Int>> entire_group = group::unordered_orbit<pm::operations::group::on_container,
                                                                    Array<Int>, Array<Int>>(homog_action_generators, Array<Int>(sequence(0,homog_action_generators[0].size())));
   if (verbosity)
      cerr << "group size: " << entire_group.size() << endl;
   if (verbosity > 2)
      cerr << "entire_group:\n" << entire_group << endl << endl;

   if (verbosity)
      cerr << "calculating dual graph of complex with " << SD_facets.size() << " facets and " << SD_labels.size() << " vertices" << endl;
   
   BigObject SC("topaz::SimplicialComplex",
                "FACETS", SD_facets);
   const Graph<> dual_graph = SC.give("DUAL_GRAPH.ADJACENCY");
   if (verbosity > 2)
      cerr << "dual graph:\n" << dual_graph << endl << endl;

   const std::vector<Int> facet_rep_indices = find_facet_rep_indices(SD_facets, subdiv_rays, index_of_ray_orbit_member, entire_group, dual_graph, verbosity);
   if (verbosity)
      cerr << "fundamental domain in first barycentric subdivision has "
           << facet_rep_indices.size() << " simplices out of " << SD_facets.size() << endl;
   if (verbosity > 2) 
      cerr << "accepted facet_rep_indices:\n" << facet_rep_indices << endl << endl
           << "corresponding to facets\n" << select(SD_facets, facet_rep_indices) << endl << endl;

   if (verbosity > 2)
      paranoid_consistency_test(SD_facets, subdiv_rays, index_of_ray_orbit_member, entire_group, facet_rep_indices);
   
   const auto squeezed_flc = squeeze_data(select(SD_facets, facet_rep_indices), SD_labels, subdiv_rays, verbosity);
   if (verbosity > 2) {
      cerr << "squeezed facets:\n" << squeezed_flc.flr.facets
           << "\n\nsqueezed labels:\n";
      for (auto it = entire<indexed>(squeezed_flc.flr.labels); !it.at_end(); ++it)
         cerr << it.index() << ": " << *it << endl;
      
      cerr << "\n\nsqueezed coordinates:\n" << squeezed_flc.coordinates 
           << "\n\nused original coordinates:\n" << squeezed_flc.used_original_coordinates << endl << endl;
   }
   
   BigObject hc_a("group::PermutationAction", "GENERATORS", homog_action_generators);
   BigObject g("group::Group", "Aut",
               "HOMOGENEOUS_COORDINATE_ACTION", hc_a);
   
   
   BigObject fd("topaz::GeometricSimplicialComplex", mlist<Scalar>(),
                "COORDINATES", squeezed_flc.coordinates,
                "FACETS", squeezed_flc.flr.facets,
                "GROUP", g);
   fd.attach("IS_FUNDAMENTAL_DOMAIN") << bool(true);
   fd.attach("VERTEX_LABELS_AS_SETS") << squeezed_flc.flr.labels;
   fd.attach("USED_ORIGINAL_COORDINATES") << squeezed_flc.used_original_coordinates;
   return fd;
}

UserFunctionTemplate4perl("# @category Symmetry"
                          "# Find a fundamental domain for a cone modulo the action of a symmetry group."
                          "# The fundamental domain will be a subcomplex, with connected DUAL_GRAPH,"
                          "# of the first barycentric subdivision that is found via a breadth-first search."
                          "# @param DisjointStackyFan F"
                          "# @return topaz::GeometricSimplicialComplex",
                          "stacky_fundamental_domain<Scalar>(DisjointStackyFan<Scalar>, { verbosity=>0 })");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
    
