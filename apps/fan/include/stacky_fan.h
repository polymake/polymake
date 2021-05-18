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

#ifndef __FAN__STACKY_FAN_H
#define __FAN__STACKY_FAN_H

#include "polymake/Array.h"
#include "polymake/Set.h"
#include "polymake/Map.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/topaz/barycentric_subdivision.h"
#include "polymake/topaz/complex_tools.h"
#include "polymake/group/permlib.h"
#include "polymake/group/induced_action.h"

namespace polymake { namespace fan {


      /*
void
validate_braid_arrangement_refines(const SparseMatrix<Rational>& ineqs)
{
   for (auto rit = entire(rows(ineqs)); !rit.at_end(); ++rit) {
      if (rit->size() > 2)
         throw std::runtime_error("Inequality matrix has more than two entries, so braid arrangement does not refine cone");
      for (auto eit = entire(*rit); !eit.at_end(); ++eit)
         if (abs(*eit) != 1)
            throw std::runtime_error("Inequality matrix contains entries distinct from +-1,0, so braid arrangement does not refine cone");
   }
}
      */

namespace {
   
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

inline   
void
check_stacky_fan(BigObject StackyFan)
{
   const Array<Set<Int>> rays_orbits = StackyFan.give("GROUP.RAYS_ACTION.ORBITS");
   const Int n_generating_rays = StackyFan.give("N_GENERATING_RAYS");

   // check that the representatives of each orbit have indices in { 0, ..., n_generating_rays-1 }
   Map<Int,Int> image_of_ray;
   for (const auto& orbit: rays_orbits) {
      const Int front = orbit.front();
      if (front >= n_generating_rays)
         throw std::runtime_error("stacky_second_bsd: Unexpected orbit representative");
      for (const auto& i: orbit)
         image_of_ray[i] = front;
   }

#if POLYMAKE_DEBUG   
   if (get_debug_level())
      cerr << "n_generating_rays: " << n_generating_rays << endl
           << "rays_orbits:\n" << rays_orbits << endl
           << "image_of_ray:\n" << image_of_ray << endl;
#endif
}

template<typename InnerLabel>   
Map<InnerLabel, InnerLabel>   
lex_min_reps_of_inners(const Array<Array<Int>>& rays_perm_generators,
                       const Array<Set<InnerLabel>>& SD_labels,
                       bool& is_any_rep_different_from_original)
{
   const group::PermlibGroup permlib_group(rays_perm_generators);
   Map<InnerLabel, InnerLabel> lex_min_rep_of;
   is_any_rep_different_from_original = false;
   for (const auto& label: SD_labels)
      for (const auto& vertex_label: label)
         if (!lex_min_rep_of.exists(vertex_label)) {
            const auto& lmr = permlib_group.lex_min_representative(vertex_label);
            if (!is_any_rep_different_from_original && lmr != vertex_label)
               is_any_rep_different_from_original = true;
            lex_min_rep_of[vertex_label] = lmr;
         }

#if POLYMAKE_DEBUG   
   if (get_debug_level())
      cerr << "lex_min_rep_of:\n" << lex_min_rep_of << endl
           << "is_any_rep_different_from_original: " << is_any_rep_different_from_original << endl;
#endif
   return lex_min_rep_of;
}

template<typename InnerLabel>   
void
identify_and_index_labels(const Array<Set<InnerLabel>>& SD_labels,
                          const Map<InnerLabel, InnerLabel>& lex_min_rep_of,
                          const IncidenceMatrix<>& VIF,
                          const bool check_id_on_bd,
                          const bool identify_only_on_boundary,
                          const bool conserve_label_cardinality,
                          Int& max_label_rep_index,
                          Map<Set<InnerLabel>, Int>& index_of_label_rep,
                          Array<Int>& rep_index_of_SD_label,
                          Map<InnerLabel, Set<InnerLabel>>& duplicate_labels_of)
{
   for (Int i=0; i<SD_labels.size(); ++i) {
      Set<InnerLabel> label_rep;
      const bool is_on_bd(topaz::on_boundary(SD_labels[i], VIF));
      if (identify_only_on_boundary && !is_on_bd)
         label_rep = SD_labels[i];
      else {
         for (const auto& vertex_label: SD_labels[i]) {
            const InnerLabel lmr(lex_min_rep_of[vertex_label]);
            const bool have_seen_rep = label_rep.collect(lmr);
            if (conserve_label_cardinality && have_seen_rep) {
               duplicate_labels_of[lmr] += vertex_label;
               label_rep += vertex_label;
            }
         }
      }

      if (check_id_on_bd && !identify_only_on_boundary && !is_on_bd && label_rep != SD_labels[i]) {
         cerr << "interior label " << SD_labels[i] << " gets id'd to " << label_rep << endl;
      }
          
      if (!index_of_label_rep.exists(label_rep))
         index_of_label_rep[label_rep] = max_label_rep_index++;
      rep_index_of_SD_label[i] = index_of_label_rep[label_rep];
   }

#if POLYMAKE_DEBUG   
   if (get_debug_level())
      cerr << "index_of_label_rep:\n" << index_of_label_rep << endl
           << "rep_index_of_SD_label:\n" << rep_index_of_SD_label << endl;
#endif
}

template<typename InnerLabel>   
void
identify_facets_and_labels(const Array<Set<Int>>& SD_facets,
                           const Array<Set<InnerLabel>>& SD_labels,
                           const bool identify_only_on_boundary,
                           const bool check_id_on_bd,
                           const Array<Array<Int>>& rays_perm_generators,
                           const IncidenceMatrix<>& VIF,
                           Set<Set<Int>>& identified_facets,
                           Array<Set<InnerLabel>>& identified_labels,
                           bool& is_any_rep_different_from_original)
{
   // Each vertex of the second barycentric subdivision is represented as a Set<Set<Int>>
   // The rays_perm_generators permute the indices of the inner Set<Int>
   // So we first have to figure out the canonical representatives of those inner Set<Int>s
   const Map<InnerLabel, InnerLabel> lex_min_rep_of = lex_min_reps_of_inners(rays_perm_generators, SD_labels, is_any_rep_different_from_original);
   
   // if the inner vertices are not permuted, we just give back the current subdivision
   if (!is_any_rep_different_from_original)
      return;

   // else we find the images of the entire labels
   Int max_label_rep_index(0);
   Map<Set<InnerLabel>, Int> index_of_label_rep;
   Array<Int> rep_index_of_SD_label(SD_labels.size());
   Map<Set<Int>,Set<Set<Int>>> duplicate_labels_of;
   const bool conserve_label_cardinality(false);
   identify_and_index_labels(SD_labels, lex_min_rep_of, VIF, check_id_on_bd, identify_only_on_boundary, conserve_label_cardinality, max_label_rep_index, index_of_label_rep, rep_index_of_SD_label, duplicate_labels_of);
   
   identified_labels.resize(max_label_rep_index);
   for (const auto& iolr: index_of_label_rep)
      identified_labels[iolr.second] = iolr.first;

#if POLYMAKE_DEBUG   
   if (get_debug_level()) {
      cerr << "identified_labels:\n";
      for (Int i=0; i<identified_labels.size(); ++i)
         cerr << i << ": " << identified_labels[i] << endl;
      cerr << endl;
   }
#endif
   
   identified_facets.clear();
   for (const auto& facet: SD_facets) {
      InnerLabel image;
      for (const auto& i: facet)
         image += rep_index_of_SD_label[i];
      identified_facets += image;
   }

#if POLYMAKE_DEBUG      
   if (get_debug_level())
      cerr << "identified_facets:\n"
           << identified_facets << endl;
#endif
}


} // end anonymous namespace


}}

#endif // __FAN__STACKY_FAN_H


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
