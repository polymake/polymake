/* Copyright (c) 1997-2016
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

#include "polymake/client.h"
#include "polymake/topaz/multi_associahedron_sphere.h"

namespace polymake { namespace topaz {

perl::Object
multi_associahedron_sphere(int n, int k)
{
   const int 
      m  (n*(n-1)/2-n*k), // the number of vertices = relevant diagonals
      dim(k*(n-2*k-1)-1); 

   DiagonalIndex index_of;
   DiagonalList diagonals; diagonals.reserve(m);
   DiagonalLabels labels; labels.reserve(m);
   prepare_diagonal_data(n, k, index_of, diagonals, labels);
   assert(index_of.size() == size_t(m));

   perl::Object Dn = group::dihedral_group_impl(2*n);
   const Array<Array<int>> gens = Dn.give("PERMUTATION_ACTION.GENERATORS");
   const auto igens(induced_action_Dn_gens_impl(gens, diagonals, index_of));

   // the complex is k-neighborly, so we know part of the f-vector
   Array<int> f_vector(dim+1);
   Array<int>::iterator fvec_it = f_vector.begin();
   *fvec_it = m; ++fvec_it;
   for (int kk=2; kk<=k; ++kk) {
      *fvec_it = convert_to<int>(Integer::binom(m,kk));
      ++fvec_it;
   }

   // we build up the face lattice in layers, starting  from representatives of the k-cliques
   hash_set<Set<int>> lower_rep_level;
   for (auto cit = entire(all_subsets_of_k(sequence(0,m), k)); !cit.at_end(); ++cit)
      lower_rep_level += *cit;

   for (int n_vertices = k+1; n_vertices <= dim+1; ++n_vertices) {
      hash_set<Set<int>> upper_rep_level;
      for (const auto& c : lower_rep_level) {
         for (int d_index=0; d_index < m; ++d_index) {
            if (c.contains(d_index))
               continue;
            if (contains_new_k_plus_1_crossing(d_index, k, c, diagonals))
               continue;
            upper_rep_level += group::orbit<group::on_container>(igens, Set<int>(c+d_index));
         }
      }
      *fvec_it = upper_rep_level.size(); ++fvec_it;
      lower_rep_level = upper_rep_level;
   }

   perl::Object induced_action("group::PermutationAction");
   induced_action.set_description("action induced by D" + std::to_string(2*n) + " on the vertices of the simplicial complex");
   induced_action.take("GENERATORS") << igens;

   const group::ConjugacyClassReps class_reps = Dn.give("PERMUTATION_ACTION.CONJUGACY_CLASS_REPRESENTATIVES");
   group::ConjugacyClassReps induced_ccr(class_reps.size());
   Entire<group::ConjugacyClassReps>::iterator iicr_it = entire(induced_ccr);
   for (const auto r: class_reps) {
      *iicr_it = induced_gen(r, diagonals, index_of);
      ++iicr_it;
   }
   induced_action.take("CONJUGACY_CLASS_REPRESENTATIVES") << induced_ccr;

   perl::Object ind_Dn("group::Group");
   ind_Dn.take("ORDER") << 2*n;
   ind_Dn.take("CHARACTER_TABLE") << group::dn_character_table(2*n);
   ind_Dn.take("PERMUTATION_ACTION") << induced_action;
      
   perl::Object s("SimplicialComplex");
   s.take("N_VERTICES") << m;
   s.take("VERTEX_LABELS") << labels;
   s.take("FACETS") << Array<Set<int>>(lower_rep_level.size(), entire(lower_rep_level));
   s.take("DIM") << dim;
   s.take("PURE") << true;
   s.take("SPHERE") << true;
   s.take("F_VECTOR") << f_vector;
   s.take("GROUP") << ind_Dn;
   return s;
}

UserFunction4perl("# @category Producing from scratch"
                  "# Produce the simplicial sphere of (k+1)-crossing free multitriangulations of an n-gon"
                  "# @param Int n the number of vertices of the polygon"
                  "# @param Int k the number of diagonals that are allowed to mutually cross"
                  "# @return SimplicialComplex",
                  &multi_associahedron_sphere,
                  "multi_associahedron_sphere($$)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
