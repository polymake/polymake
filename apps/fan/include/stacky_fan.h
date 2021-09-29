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

#define MY_POLYMAKE_DEBUG 1

#ifndef __FAN__STACKY_FAN_H
#define __FAN__STACKY_FAN_H

#include "polymake/Array.h"
#include "polymake/Set.h"
#include "polymake/Map.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/topaz/barycentric_subdivision.h"
#include "polymake/topaz/complex_tools.h"
#include "polymake/topaz/boundary_tools.h"
#include "polymake/group/permlib.h"
#include "polymake/group/induced_action.h"
#include "polymake/group/orbit.h"

/*
  Conventions:

- SD_facets and SD_labels come from the first or second barycentric subdivision of the original stacky fan
  * SD_facets is an Array<Set<Int>> whose integer entries refer to the SD_labels
  * SD_labels is an Array<Set<Set<Int>>> 
    - in the case of the first barycentric subdivision, the Set<Set>s are Sets of singletons that refer to the rays of the original fan
      (they come first in the list of orbits in the orbit fan)
    - in the case of the second barycentric subdivisions, the singletons are replaced by sets 
 */

namespace polymake { namespace fan {

typedef std::list<Set<Int>> OrbitCollection;
typedef std::vector<Int> ContractedRayIndices;

template<typename Scalar>   
struct SubdivisionData {
   topaz::FacetsAndLabels fal;
   Matrix<Scalar> subdiv_rays;
};
      
   
template<typename Container>
Array<std::string>
make_strings(const Container& labels)
{
   Array<std::string> label_strings(labels.size());
   std::ostringstream os;
   auto ils_it = entire(label_strings);
   for (const auto& lset: labels) {
      wrap(os) << lset;
      *ils_it = os.str();
      ++ils_it;
      os.str("");
   }
   return label_strings;
}

      
template<typename LabelType>      
struct FacetsLabelsReordering {
   Array<Set<Int>> facets;
   Array<Int> reordering;
   LabelType labels;
};
   
template<typename FacetType, typename LabelType>      
FacetsLabelsReordering<LabelType>
squeeze_facets_and_labels(const FacetType& facets,
                          const LabelType& labels,
                          const Int verbosity)
{
   FacetsLabelsReordering<LabelType> squeezed_flr;
   IncidenceMatrix<> reordered_facets(facets);
   const auto& squeezed_pair = topaz::squeeze_faces(reordered_facets);
   if (verbosity > 2) {
      cerr << "squeezing vertices to\n";
      for (auto it = entire<indexed>(squeezed_pair.second); !it.at_end(); ++it)
         if (*it != it.index())
            cerr << "(" << *it << "->" << it.index() << ") ";
      cerr << endl << endl;
   }
   
   squeezed_flr.facets = squeezed_pair.first;
   squeezed_flr.reordering = squeezed_pair.second;
   squeezed_flr.labels = LabelType(squeezed_pair.second.size());
   
   auto rl_it = entire(squeezed_flr.labels);
   for (const Int i: squeezed_pair.second) {
      *rl_it = labels[i];
      ++rl_it;
   }
   return squeezed_flr;
}
            
                                     

template<typename Scalar, bool store_whole_orbit=false>
class Second_SD_RayOrbitStorer {
   using Storage = std::vector<std::pair<hash_set<Vector<Scalar>>,
                                         Set<Int>>>;
protected:
   const Array<Array<Int>>& generators_or_group;
   Storage orbit_and_contained_indices;
   const bool is_entire_group_stored;

public:
   Second_SD_RayOrbitStorer(const Array<Array<Int>>& gens)
      : generators_or_group(gens)
      , orbit_and_contained_indices()
      , is_entire_group_stored(false)
   {}

   Second_SD_RayOrbitStorer(const Array<Array<Int>>& gens, bool _is_entire_group_stored)
      : generators_or_group(gens)
      , orbit_and_contained_indices()
      , is_entire_group_stored(_is_entire_group_stored)
   {}

private:
   bool is_found(const Vector<Scalar>& v, const Int index) {
      for (auto& oi: orbit_and_contained_indices) {
         if (oi.first.contains(v)) {
            oi.second += index;
            return true;
         }
      }
      return false;
   }

public:
   
   template<bool trigger = store_whole_orbit>
   std::enable_if_t<!trigger>
   add(const Vector<Scalar>& v,
       const Int index) {
      if (is_found(v, index))
         return;
      
      // haven't found it yet, so v spawns a new orbit
      const auto&& the_orbit = group::unordered_orbit<pm::operations::group::on_container, Array<Int>, Vector<Scalar>>(generators_or_group, v);
      orbit_and_contained_indices.push_back(std::make_pair(the_orbit, scalar2set(index)));
   }

   template<bool trigger = store_whole_orbit>
   std::enable_if_t<trigger>
   add(const Vector<Scalar>& v,
       const Int index) {
      if (is_found(v, index))
         return;
   
      // haven't found it yet, so v spawns a new orbit
      hash_set<Vector<Scalar>> the_orbit;
      for (const auto& g: generators_or_group) {
         const pm::operations::group::action<Vector<Scalar>, pm::operations::group::on_container, Array<Int>> this_action(g);
         the_orbit += this_action(v);
      }
      orbit_and_contained_indices.push_back(std::make_pair(the_orbit, scalar2set(index)));
   }

   const Storage& get_storage() const { return orbit_and_contained_indices; }
};

inline
Set<Int>
do_identification(const Set<Int>& f,
                  const Array<Int>& identifies_to)
{
   Set<Int> identified_f;
   for (const Int i: f)
      identified_f += identifies_to[i];
   return identified_f;
}
      
inline
void
do_label_substitution(Array<Set<Set<Int>>>& SD_labels,
                      const Array<Set<Set<Int>>>& original_labels)
{
   for (auto& label: SD_labels) {
      assert(label.size() == 1);
      Set<Set<Int>> subs_label;
      for (const Int i: label.front()) {
         assert(original_labels[i].size() == 1);
         subs_label += original_labels[i].front();
      }
      label = subs_label;
   }
}


template<typename IncomingOrbit>
void include_in_collection(OrbitCollection& orbit_collection,
                           const IncomingOrbit& incoming_orbit,
                           const Int verbosity)
{
   Set<Int> collected_orbit(incoming_orbit);
   std::vector<Set<Int>> collated_sets;
   orbit_collection.remove_if([&collected_orbit, &incoming_orbit, &collated_sets, verbosity](const Set<Int>& existing_orbit){
                                 if ((incoming_orbit * existing_orbit).size()) {
                                    // there is some overlap, so we have to remove existing_orbit from the list
                                    // even though it might be the only set with overlap,
                                    // because we don't know yet how many more existing_orbits with overlap to incoming_orbit there will be
                                    collected_orbit += existing_orbit;
                                    if (verbosity > 2)
                                       collated_sets.push_back(existing_orbit);
                                    return true;
                                 }
                                 return false;
                              });
   orbit_collection.push_back(collected_orbit);
   if (verbosity > 2 && collated_sets.size()) {
      cerr << "include_in_collection: current size " << orbit_collection.size()
           << ". incoming orbit " << incoming_orbit << " collated ";
      for (const auto& s: collated_sets)
         cerr << s << " ";
      cerr << " to " << collected_orbit << endl;
   }
}

template<typename OrbitCollectionType>
Array<Int>
convert_to_identification_map(const OrbitCollectionType& orbit_collection,
                              const Int n,
                              const Int verbosity)
{
   Array<Int> identifies_to(sequence(0,n));

   for (const auto& orbit: orbit_collection)
      if (orbit.size() > 1)
         for (const Int i: orbit)
            identifies_to[i] = orbit.front();
   if (verbosity > 2) {
      cerr << "identifies_to:\n";
      for (auto it = entire<indexed>(identifies_to); !it.at_end(); ++it)
         if (*it != it.index())
            cerr << "(" << it.index() << "->" << *it << ") ";
      cerr << endl << endl;
   }

   return identifies_to;
}

template<typename OrbitCollectionType>
void
paranoid_orbit_collection_check(const OrbitCollectionType& orbit_collection)
{
   for (auto it1 = entire(orbit_collection); !it1.at_end(); ++it1) {
      auto it2 = it1;
      ++it2;
      while (!it2.at_end()) {
         if (((*it1)*(*it2)).size()) {
            cerr << "paranoid_orbit_collection_check: oops. " << *it1 << ", " << *it2 << endl;
            throw std::runtime_error("stop");
         }
         ++it2;
      }
   }
}

template<typename Scalar>      
Matrix<Scalar>
subdivision_rays(const Matrix<Scalar>& original_rays,
                 const Array<Set<Set<Int>>>& labels,
                 const Int verbosity)
{
   const Int n(labels.size());
   Matrix<Scalar> subdiv_rays(n, original_rays.cols());
   for (Int i=0; i<n; ++i) 
      subdiv_rays[i] = accumulate(rows(original_rays.minor(labels[i].front(), All)), operations::add());
   if (verbosity > 2) {
      cerr << "rays of bsd:\n";
      for (Int i=0; i<n; ++i)
         cerr << i << ": " << subdiv_rays[i] << endl;
      cerr << endl;
   }
   return subdiv_rays;
}

template<typename Scalar, typename MapType, typename ContractedEdgesType>
ContractedRayIndices
contract_ray_indices(const Matrix<Scalar>& subdiv_rays,
                     const MapType& coordinate_of_unmarked_edge,
                     const ContractedEdgesType& contracted_edges)
{
   ContractedRayIndices contracted_ray_indices;
   for (auto rit = entire<indexed>(rows(subdiv_rays)); !rit.at_end(); ++rit) {
      bool accept_row(true);
      for (const Int i: contracted_edges) {
         const auto find_it = coordinate_of_unmarked_edge.find(i);
         if (find_it == coordinate_of_unmarked_edge.end())
            throw std::runtime_error("contracted_ray_indices: edge index not found");
         if (!is_zero((*rit)[find_it->second])) {
            accept_row = false;
            break;
         }
      }
      if (accept_row)
         contracted_ray_indices.push_back(rit.index());
   }
   return contracted_ray_indices;
}

template<typename Scalar>
void
include_orbits_in_collection(OrbitCollection& orbit_collection,
                             Second_SD_RayOrbitStorer<Scalar>& ros,
                             const ContractedRayIndices& contracted_ray_indices,
                             const Matrix<Scalar>& subdiv_rays,
                             const Int verbosity)
{
   for (Int i: contracted_ray_indices)
      ros.add(Vector<Scalar>(subdiv_rays[i]), i);
   if (verbosity > 3) {
      cerr << "orbits_on_contracted_face (" << ros.get_storage().size() << "):\n";
      for (const auto& oi: ros.get_storage())
         cerr << oi.first << "; " << oi.second << endl;
      cerr << endl;
   }
      
   for (const auto& orbit_and_indices: ros.get_storage())
      include_in_collection(orbit_collection, orbit_and_indices.second, verbosity);
}   

template<typename LabelType>      
auto
identify_squeeze_and_substitute(const topaz::FacetsAndLabels& fal,
                                const LabelType& original_labels,
                                const Array<Int>& identifies_to,
                                const Int verbosity)
{
   Set<Set<Int>> identified_facets;
   for (const auto& f: fal.facets) 
      identified_facets += do_identification(f, identifies_to);

   if (verbosity)
      cerr << fal.facets.size() << " facets before, " << identified_facets.size() << " after identification" << endl;
   if (verbosity > 3)
      cerr << "\nidentified facets (" << identified_facets.size() << "):\n" << identified_facets << endl << endl;

   auto squeezed_fal = squeeze_facets_and_labels(identified_facets, fal.labels, verbosity);

   if (verbosity > 3) 
      cerr << "squeezed facets (" << squeezed_fal.facets.size() << "):\n"
           << squeezed_fal.facets
           << "\n\nlabels before substitution:\n" << squeezed_fal.labels
           << "\nwill substitute with:\n" << original_labels
           << endl;
   
   do_label_substitution(squeezed_fal.labels, original_labels);
   if (verbosity > 3) {
      cerr << "\nsubstituted_labels:\n";
      for (Int i=0; i<squeezed_fal.labels.size(); ++i)
         cerr << i << ": " << squeezed_fal.labels[i] << endl;
   }
   return squeezed_fal;
}

}}

#endif // __FAN__STACKY_FAN_H


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
    
