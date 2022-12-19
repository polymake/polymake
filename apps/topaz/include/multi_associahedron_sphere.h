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

#pragma once

#include "polymake/hash_map"
#include "polymake/hash_set"
#include "polymake/Set.h"
#include "polymake/PowerSet.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/group/orbit.h"
#include "polymake/group/named_groups.h"
#include "polymake/topaz/boundary_tools.h"
#include <algorithm>

namespace polymake { namespace topaz {

namespace multi_associahedron_sphere_utils {

using Diagonal = std::pair<Int, Int>;
using DiagonalIndex = hash_map<Diagonal, Int> ;
using DiagonalList = std::vector<Diagonal>;
using DiagonalLabels = std::vector<std::string>;

enum IncludeCrossings : bool {   
   yes = true,                             
   no = false
};

bool inside(Int x, const Diagonal& d)
{
   return x > d.first && x < d.second;
}

bool cross(const Diagonal& d1, const Diagonal& d2)
{
   assert(d1.first < d1.second && d2.first < d2.second);

   if (d1.first==d2.first || d1.second==d2.second)
      return false;

   const Int v0 = std::min(d1.first, d2.first);

   const Diagonal d1t{ d1.first - v0, d1.second - v0 },
                  d2t{ d2.first - v0, d2.second - v0 };
      
   return 
      inside(d2t.first, d1t) && !inside(d2t.second, d1t) ||
      inside(d2t.second, d1t) && !inside(d2t.first, d1t);
}

bool cross_mutually(const Set<Int>& c,
                    const DiagonalList& diagonals)
{
   assert(c.size() >= 2);
   for (auto pit = entire(all_subsets_of_k(c,2)); !pit.at_end(); ++pit)
      if (!cross(diagonals[(*pit).front()], diagonals[(*pit).back()]))
         return false;
   return true;
}

bool crosses_all(Int new_d,
                 const Set<Int>& family,
                 const DiagonalList& diagonals)
{
   for (const auto& i: family)
      if (!cross(diagonals[new_d], diagonals[i]))
         return false;
   return true;
}

bool contains_new_k_plus_1_crossing(Int new_d,
                                    Int k,
                                    const Set<Int>& family,
                                    const DiagonalList& diagonals)
{
   if (k > 1) {
      if (family.size() < k)
         return false;
      for (auto kc_it = entire(all_subsets_of_k(family, k)); !kc_it.at_end(); ++kc_it) {
         if (!crosses_all(new_d, *kc_it, diagonals)) // first, a linear test
            continue;
         if (cross_mutually(*kc_it, diagonals)) // then a quadratic one
            return true;
      }
      return false;
   } else { // k==1
      for (auto f_it = entire(family); !f_it.at_end(); ++f_it)
         if (cross(diagonals[new_d], diagonals[*f_it]))
            return true;
      return false;
   }
}

void
fill_upper_layer(hash_set<Set<Int>>& upper_layer,
                 const hash_set<Set<Int>>& lower_layer,
                 const Int k,
                 const Int m,
                 const DiagonalList& diagonals,
                 const Array<Array<Int>>& igens,
                 const Set<Int>& link_of,
                 const IncludeCrossings include_crossings)
{
   for (const auto& F: lower_layer) {
      for (Int d_index = 0; d_index < m; ++d_index) {
         if (!F.contains(d_index) &&
             include_crossings ==
             contains_new_k_plus_1_crossing(d_index, k, F, diagonals)) {
            for (const auto& G: group::unordered_orbit<group::on_container>(igens, Set<Int>(F+d_index))) {
               if (!link_of.size() ||
                   incl(link_of, G) <= 0) {
                  upper_layer += G;
               }
            }
         }
      }
   }
}   
   
Array<Int> induced_gen(const Array<Int>& g,
                       const DiagonalList& diagonals,
                       const DiagonalIndex& index_of)
{
   Array<Int> induced_gen(diagonals.size());
   auto igit = entire(induced_gen);
   for (const auto& d: diagonals) {
      const Diagonal unordered_img{ g[d.first], g[d.second] };
      const Diagonal img = unordered_img.first < unordered_img.second
                           ? unordered_img
                           : Diagonal{ unordered_img.second, unordered_img.first };
      *igit = index_of.at(img);
      ++igit;
   }
   return induced_gen;
}

Array<Array<Int>>
induced_action_gens_impl(const Array<Array<Int>>& gens,
                         const DiagonalList& diagonals,
                         const DiagonalIndex& index_of)
{
   Array<Array<Int>> igens(gens.size());
   std::transform(gens.begin(), gens.end(), igens.begin(),
                  [&diagonals, &index_of](auto gen) { return induced_gen(gen, diagonals, index_of); });
   return igens;
}

void prepare_diagonal_data(Int n,
                           Int k,
                           DiagonalIndex& index_of,
                           DiagonalList& diagonals,
                           DiagonalLabels& labels)
{
   Int index = -1;
   std::ostringstream oss;
   for (Int steps = k+1; steps <= n/2; ++steps) {
      for (Int u = 0; u<n; ++u) {
         if (n%2==0 && steps==n/2 && u==n/2)
            break; // for even n-gons, the big diagonals must be counted only once
         const Int v = (u+steps) % n;
         const Diagonal d{ std::min(u, v), std::max(u, v) };
         index_of[d] = ++index;
         diagonals.push_back(d);
         oss.str("");
         wrap(oss) << "(" << d << ")";
         labels.push_back(oss.str());
      }
   }
}


void
dihedral_group_action(const Int n,
                      const DiagonalList& diagonals,
                      DiagonalIndex& index_of,
                      BigObject& ind_Aut,
                      BigObject& induced_action,
                      Array<Array<Int>>& igens)
{
   BigObject Aut = group::dihedral_group(2*n);
   const Array<Array<Int>> gens = Aut.give("PERMUTATION_ACTION.GENERATORS");
   igens = induced_action_gens_impl(gens, diagonals, index_of);
   
   induced_action.set_description("action of D_" + std::to_string(2*n)
                                  + " on the vertices of the simplicial complex, induced by the action of D_"
                                  + std::to_string(2*n) + " on the vertices of the polygon");
   
   const group::ConjugacyClassReps<> class_reps = Aut.give("PERMUTATION_ACTION.CONJUGACY_CLASS_REPRESENTATIVES");
   group::ConjugacyClassReps<> induced_ccr(class_reps.size());
   auto iicr_it = entire(induced_ccr);
   for (const auto& r: class_reps) {
      *iicr_it = induced_gen(r, diagonals, index_of);
      ++iicr_it;
   }
   induced_action.take("CONJUGACY_CLASS_REPRESENTATIVES") << induced_ccr;
   ind_Aut.take("CHARACTER_TABLE") << group::dn_character_table(2*n);
   ind_Aut.take("ORDER") << 2*n;
}

void
symmetric_group_action(const Int m,
                       const Int n,
                       DiagonalIndex& index_of,
                       BigObject& ind_Aut,
                       BigObject& induced_action,
                       Array<Array<Int>>& igens)
{
   induced_action.set_description("action of S" + std::to_string(m)
                                  + " on the vertices of the simplicial complex, induced by the action of D_"
                                  + std::to_string(2*n) + " on the vertices of the polygon");
   igens = group::symmetric_group_gens(m);
   if (m<8) {
      induced_action.take("CONJUGACY_CLASS_REPRESENTATIVES") << group::sn_reps(m);
      ind_Aut.take("CHARACTER_TABLE") << group::sn_character_table(m);
   }
   ind_Aut.take("ORDER") << Integer::fac(m);
}

void
initialize_f_vector(Array<Int>::iterator& fvec_it,
                    const Int m,
                    const Int k)
{
   // the complex is k-neighborly, so we know part of the f-vector
   *fvec_it++ = m;
   for (Int kk = 2; kk <= k; ++kk)
      *fvec_it++ = Int(Integer::binom(m,kk));
}

void
squeeze_matrix(IncidenceMatrix<>& facets,
               DiagonalLabels& labels,
               const hash_set<Set<Int>>& lower_layer,
               const Set<Int>& link_of)
{
   facets.resize(lower_layer.size(), labels.size());
   auto lit = entire(lower_layer);
   for (auto rit = entire(rows(facets)); !rit.at_end(); ++rit, ++lit)
      *rit = Set<Int>(*lit - link_of);
   
   auto c = ind2map_consumer(facets.cols());
   facets.squeeze_cols(c);

   const Array<Int> index_map(facets.cols(), entire(c.give_map()));
   DiagonalLabels squeezed_labels(facets.cols());
   for (auto it = entire<indexed>(index_map); !it.at_end(); ++it) 
      squeezed_labels[it.index()] = labels[*it];

   labels.swap(squeezed_labels);
}

} } }


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:


