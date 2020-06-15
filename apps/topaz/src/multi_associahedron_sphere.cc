/* Copyright (c) 1997-2020
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
#include "polymake/Integer.h"
#include "polymake/topaz/multi_associahedron_sphere.h"

namespace polymake { namespace topaz {

using namespace multi_associahedron_sphere_utils;

BigObject
multi_associahedron_sphere(Int n, Int k, OptionSet options)
{
   const Int 
      m  (n*(n-1)/2-n*k), // the number of vertices = relevant diagonals
      nu (n-2*k-1),       // the complexity measure
      dim(k*nu-1);        // the dimension of the sphere

   const bool no_facets    = options["no_facets"];
   const bool no_crossings = options["no_crossings"];
   
   if (k<=0) throw std::runtime_error("multi_associahedron_sphere: require k>=1");
   if (m<=0) throw std::runtime_error("multi_associahedron_sphere: require (n choose 2) > n*k");
   assert(n>=3);

   DiagonalIndex index_of;
   DiagonalList diagonals; diagonals.reserve(m);
   DiagonalLabels labels; labels.reserve(m);
   prepare_diagonal_data(n, k, index_of, diagonals, labels);
   assert(index_of.size() == size_t(m));
   
   BigObject ind_Aut("group::Group");
   BigObject induced_action("group::PermutationAction");
   Array<Array<Int>> igens;
   
   if (nu > 1) {
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
   } else {
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

   induced_action.take("GENERATORS") << igens;
   ind_Aut.take("PERMUTATION_ACTION") << induced_action;
      
   hash_set<Set<Int>> lower_rep_level, k_plus_1_crossings;

   if (!no_crossings || !no_facets) {
      // accumulate all k-cliques, to prepare for calculating the (k+1)-crossings and the f-vector
      for (auto cit = entire(all_subsets_of_k(sequence(0,m), k)); !cit.at_end(); ++cit)
         lower_rep_level += *cit;
   }

   if (!no_crossings) {
      // now calculate the (k+1)-crossings
      for (const auto& c : lower_rep_level)
         for (Int d_index = 0; d_index < m; ++d_index)
            if (!c.contains(d_index) && contains_new_k_plus_1_crossing(d_index, k, c, diagonals))
               k_plus_1_crossings += group::unordered_orbit<group::on_container>(igens, Set<Int>(c+d_index));
   }
   
   Array<Int> f_vector(dim+1);

   if (!no_facets) {
      // the complex is k-neighborly, so we know part of the f-vector
      auto fvec_it = f_vector.begin();
      *fvec_it++ = m;
      for (Int kk = 2; kk <= k; ++kk)
         *fvec_it++ = Int(Integer::binom(m,kk));

      // build up the face lattice in layers, starting from representatives of the k-cliques
      for (Int n_vertices = k+1; n_vertices <= dim+1; ++n_vertices) {
         hash_set<Set<Int>> upper_rep_level;
         for (const auto& c : lower_rep_level)
            for (Int d_index = 0; d_index < m; ++d_index)
               if (!c.contains(d_index) && !contains_new_k_plus_1_crossing(d_index, k, c, diagonals))
                  upper_rep_level += group::unordered_orbit<group::on_container>(igens, Set<Int>(c+d_index));

         *fvec_it++ = upper_rep_level.size();
         lower_rep_level = upper_rep_level;
      }
   }
   
   BigObject s("SimplicialComplex");
   s.set_description() << "Simplicial complex of " << k << "-triangulations of a convex " << n << "-gon" << endl;
   s.take("N_VERTICES") << m;
   s.take("VERTEX_LABELS") << labels;
   s.take("DIM") << dim;
   s.take("PURE") << true;
   s.take("SPHERE") << true;
   s.take("GROUP") << ind_Aut;

   if (!no_crossings) {
      s.attach("K_PLUS_1_CROSSINGS") << Set<Set<Int>>(entire(k_plus_1_crossings));
   }
   
   if (!no_facets) {
      s.take("FACETS") << Set<Set<Int>>(entire(lower_rep_level));
      s.take("F_VECTOR") << f_vector;
   }
   
   return s;
}
      
UserFunction4perl("# @category Producing from scratch"
                  "# Produce the simplicial sphere //Δ(n,k)// of (//k//+1)-crossing free multitriangulations"
                  "# of an //n//-gon //P//, along with the group action on the diagonals induced from //D//_{2//n//}."
                  "# //Δ(n,k)// is the simplicial complex on the set of relevant diagonals of //P// whose faces are those sets"
                  "# of diagonals such that no //k//+1 of them mutually cross. A diagonal is //relevant// if it leaves"
                  "# //k// or more vertices of //P// on both sides. (Any diagonal having less than //k// vertices on one"
                  "# side trivially cannot participate in a (//k//+1)-crossing, so is //irrelevant//. The corresponding"
                  "# complex on //all// diagonals is therefore the simplicial join of this one with the simplex of irrelevant"
                  "# diagonals.)"
                  "# \t Jakob Jonsson, \"Generalized triangulations and diagonal-free subsets of stack polyominoes\","
                  "# \t J. Combin. Theory Ser. A, 112(1):117–142, 2005"
                  "# //Δ(n,k)// is known to be a //k//-neighborly vertex-decomposable sphere of dimension //k//ν-1,"
                  "# where the parameter ν=//n//-2//k//-1 measures the complexity of this construction."
                  "# For ν=0, the complex is a point; for ν=1 a //k//-simplex; for ν=2 the boundary of a cyclic polytope."
                  "# Setting //k//=1 yields the boundary of the simplicial associahedron."
                  "# The list of (//k//+1)-crossings in the //n//-gon is included as the attachment K_PLUS_1_CROSSINGS. It can"
                  "# also be obtained as the property MINIMAL_NON_FACES, but this requires the HASSE_DIAGRAM to be computed."
                  "# @param Int n the number of vertices of the polygon"
                  "# @param Int k the number of diagonals that are allowed to mutually cross"
                  "# @option Bool no_facets don't calculate the facets (for large examples)? Default 0"
                  "# @option Bool no_crossings don't calculate the crossings? Default 0"
                  "# @return SimplicialComplex"
                  "# @example The f-vector of Δ(9,3) is that of a neighborly polytope, since ν=2:"
                  "# > print multi_associahedron_sphere(9,3)->F_VECTOR;"
                  "# | 9 36 84 117 90 30"
                  "# @example The option no_facets=>1 still leaves useful information:"
                  "# > $s = multi_associahedron_sphere(8,2, no_facets=>1);"
                  "# > print $s->VERTEX_LABELS;"
                  "# | (0 3) (1 4) (2 5) (3 6) (4 7) (0 5) (1 6) (2 7) (0 4) (1 5) (2 6) (3 7)"
                  "# > print $s->GROUP->PERMUTATION_ACTION->GENERATORS;"
                  "# | 7 0 1 2 3 4 5 6 11 8 9 10"
                  "# | 4 3 2 1 0 7 6 5 11 10 9 8"
                  "# > print $s->get_attachment(\"K_PLUS_1_CROSSINGS\")->size();"
                  "# | 28"
                  ,
                  &multi_associahedron_sphere,
                  "multi_associahedron_sphere($$ { no_facets=>0, no_crossings=>0 })");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
