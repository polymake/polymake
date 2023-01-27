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

   const bool include_facets    = options["include_facets"];
   const Array<Set<Int>> link_of_diagonals = options["link_of_diagonals"];
   const bool include_crossings = link_of_diagonals.size()
      ? false
      : options["include_crossings"];
   
   if (n<=2) throw std::runtime_error("multi_associahedron_sphere: require n>=3");
   if (k<=0) throw std::runtime_error("multi_associahedron_sphere: require k>=1");
   if (m<=0) throw std::runtime_error("multi_associahedron_sphere: require (n choose 2) > n*k");

   DiagonalIndex index_of;
   DiagonalList diagonals; diagonals.reserve(m);
   DiagonalLabels labels; labels.reserve(m);
   prepare_diagonal_data(n, k, index_of, diagonals, labels);
   assert(index_of.size() == size_t(m));

   Set<Int> link_of;
   for (const auto& d: link_of_diagonals) {
      if (2 != d.size())
         throw std::runtime_error("multi_associahedron_sphere: diagonals in option link_of_diagonals must have size 2");
      const auto key(std::make_pair(d.front(), d.back()));
      if (!index_of.exists(key))
         throw std::runtime_error("multi_associahedron_sphere: diagonal given in link_of_diagonals is not actually a diagonal");
      link_of += index_of[key];
   }
   
   BigObject ind_Aut("group::Group");
   BigObject induced_action("group::PermutationAction");
   Array<Array<Int>> igens;

   if (!link_of.size()) {
      if (nu > 1) {
         dihedral_group_action(n, diagonals, index_of, ind_Aut, induced_action, igens);
      } else {
         symmetric_group_action(m, n, index_of, ind_Aut, induced_action, igens);
      }
   
      induced_action.take("GENERATORS") << igens;
      ind_Aut.take("PERMUTATION_ACTION") << induced_action;
   }
      
   hash_set<Set<Int>> lower_layer, k_plus_1_crossings;

   /*
     We build up the faces of the simplicial complex dimension by dimension by
     - maintaining a lower and an upper layer in the Hasse diagram,
     - initializing the lower layer to be set of all //k//-crossings,
     - deducing the next upper layer by
        -- for each face F in the lower layer
        -- add each possible new vertex index to it
        -- if that set contains no //k+1//-crossing, add it to the next layer
     - finally, setting lower_layer = upper_layer
     - and repeating until the number of vertices prescribed by the dimension
       is reached.

     There is another complication in that we sometimes want to calculate
     just the link of a face F = link_of, because the sphere might get too big otherwise.
     
     If that is the case, we seed lower_layer 
     - not with all elements of [n] choose k,
     - but only with the set F,
     and at the very end, we remove F from all faces.
    */

   Int current_simplex_size(-1);
   
   // initialize the lower_layer
   if (include_crossings || include_facets) {
      // accumulate all k-cliques, to prepare for calculating the (k+1)-crossings and the f-vector
      if (link_of.size()) {
         lower_layer += link_of;
         current_simplex_size = link_of.size();
      } else {
         for (auto cit = entire(all_subsets_of_k(sequence(0,m), k)); !cit.at_end(); ++cit) 
            lower_layer += *cit;
         current_simplex_size = k;
      }
   }
   
   if (include_crossings) 
      // now calculate the (k+1)-crossings
      fill_upper_layer(k_plus_1_crossings, lower_layer, k, m, diagonals, igens, link_of, IncludeCrossings::yes);

   Array<Int> f_vector(dim+1);
   auto fvec_it = f_vector.begin();
   
   if (include_facets) {
      if (!link_of.size())
         initialize_f_vector(fvec_it, m, k);

      // build up the face lattice in layers, starting from representatives of the k-cliques
      for (++current_simplex_size; current_simplex_size <= dim+1; ++current_simplex_size) {
         hash_set<Set<Int>> upper_layer;
         fill_upper_layer(upper_layer, lower_layer, k, m, diagonals, igens, link_of, IncludeCrossings::no);
         
         if (!link_of.size())
            *fvec_it++ = upper_layer.size();

         lower_layer = upper_layer;
      }
   }

   IncidenceMatrix<> facets;
   if (!link_of.size())
      facets = IncidenceMatrix<>(lower_layer);
   else 
      squeeze_matrix(facets, labels, lower_layer, link_of);
   
   BigObject s("SimplicialComplex");
   std::ostringstream os;
   if (link_of.size()) {
      wrap(os) << "Link of face "
               << link_of
               << " = ";
      for (const auto& d: link_of_diagonals)
         wrap(os) << d << " ";
      os << "of ";
   }
   os << "Simplicial complex of " << k << "-triangulations of a convex " << n << "-gon";
   s.set_description() << os.str() << endl;
   s.take("N_VERTICES") << labels.size();
   s.take("VERTEX_LABELS") << labels;
   s.take("DIM") << dim - link_of.size();
   s.take("PURE") << true;
   s.take("SPHERE") << true;

   if (!link_of_diagonals.size())
      s.take("GROUP") << ind_Aut;

   if (include_crossings) {
      s.attach("K_PLUS_1_CROSSINGS") << Set<Set<Int>>(entire(k_plus_1_crossings));
   }
   
   if (include_facets) {
      const Set<Set<Int>> facet_set(entire(rows(facets)));
      s.take("N_FACETS") << facet_set.size();
      s.take("FACETS") << facet_set;
      if (!link_of_diagonals.size())
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
                  "# @option Bool include_facets calculate the facets (for large examples)? Default 1"
                  "# @option Bool include_crossings calculate the crossings? Default 1"
                  "# @option Array<Set<Int>> link_of_diagonals calculate the link of the sphere of the given diagonals."
                  "# This option implies include_crossings=>0 and causes no GROUP to be generated"
                  "# @return SimplicialComplex"
                  "# @example The f-vector of Δ(9,3) is that of a neighborly polytope, since ν=2:"
                  "# > print multi_associahedron_sphere(9,3)->F_VECTOR;"
                  "# | 9 36 84 117 90 30"
                  "# @example The option include_facets=>0 still leaves useful information:"
                  "# > $s = multi_associahedron_sphere(8,2, include_facets=>0);"
                  "# > print $s->VERTEX_LABELS;"
                  "# | (0 3) (1 4) (2 5) (3 6) (4 7) (0 5) (1 6) (2 7) (0 4) (1 5) (2 6) (3 7)"
                  "# > print $s->GROUP->PERMUTATION_ACTION->GENERATORS;"
                  "# | 7 0 1 2 3 4 5 6 11 8 9 10"
                  "# | 4 3 2 1 0 7 6 5 11 10 9 8"
                  "# > print $s->get_attachment(\"K_PLUS_1_CROSSINGS\")->size();"
                  "# | 28"
                  ,
                  &multi_associahedron_sphere,
                  "multi_associahedron_sphere($$ { include_facets=>1, include_crossings=>1, link_of_diagonals=>[] })");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
