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
#include "polymake/linalg.h"
#include "polymake/Array.h"
#include "polymake/hash_set"
#include "polymake/Bitset.h"
#include "polymake/FacetList.h"
#include "polymake/polytope/is_regular.h"
#include "polymake/polytope/solve_LP.h"
#include "polymake/fan/intersection.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/RandomGenerators.h"

namespace polymake { namespace fan {

namespace {

typedef Bitset Simplex;   
typedef hash_set<Simplex> Subdivision;

auto
star_of(const Simplex& F, const Subdivision& S)
{
   std::vector<Simplex> star;
   for (const auto& sigma: S)
      if (incl(F, sigma) <= 0)
         star.push_back(sigma);
   if (!star.size())
      star.push_back(Simplex());
   return star;
}

auto
boundary_of(const std::vector<Simplex>& S, const Simplex& ignore)
{
   hash_map<Simplex, Int> multiplicity_of;
   for (const auto& sigma: S)
      for (const Int i: sigma)
         ++multiplicity_of[sigma - i];
   
   std::vector<Simplex> boundary;
   for (const auto& m: multiplicity_of)
      if (m.second == 1 && incl(ignore,m.first) > 0)
         boundary.push_back(m.first);
   return boundary;
}

auto
join_of(const Simplex& A, const std::vector<Simplex>& B)
{
   Subdivision join;
   for (const auto& sigma: B)
      join += sigma + A; // each such simplex can arise several times, hence the use of Set
   return join;
}

template<typename Scalar>
Subdivision
find_initial_subdivision(const Matrix<Scalar>& V_full,
                         const SparseMatrix<Scalar>& restrict_to,
                         const RandomSeed& seed)
{
   UniformlyRandom<Rational> random(seed);
   const Int n = V_full.rows();
   const Int d = V_full.cols();
   
   Vector<Scalar> heights(n);
   if (!restrict_to.rows())
      copy_range(random.begin(), entire(heights));
   else {
      const SparseMatrix<Scalar> basis = null_space(restrict_to);
      Vector<Scalar> coeffs(basis.rows());
      copy_range(random.begin(), entire(coeffs));
      heights = coeffs * basis;
   }

   BigObject Q("Polytope", mlist<Scalar>(), "POINTS", V_full | heights);

   const IncidenceMatrix<> pif = Q.give("POINTS_IN_FACETS");
   const Matrix<Scalar> F = Q.give("FACETS");

   Subdivision S;
   for (Int i = 0; i < pif.rows(); ++i)
      if (F(i,d) < Scalar(0))
         S += Simplex(pif[i]);
   return S;
}

template<typename Scalar, typename Matrix1, typename Matrix2>
std::pair<SparseMatrix<Scalar>, SparseMatrix<Scalar>>
vertices_from_ineqs(const GenericMatrix<Matrix1, Scalar>& inequalities,
                    const GenericMatrix<Matrix2, Scalar>& equations)
{
   BigObject C("Cone", mlist<Scalar>(), "INEQUALITIES", inequalities.top(), "EQUATIONS", equations.top());

   SparseMatrix<Scalar> rays = C.give("RAYS");
   SparseMatrix<Scalar> lineality = C.give("LINEALITY_SPACE");
   orthogonalize(entire(rows(lineality)));
   project_to_orthogonal_complement(rays, lineality);
   return std::make_pair(rays, lineality);
}
   
template<typename Scalar>
std::pair<SparseMatrix<Scalar>, SparseMatrix<Scalar>>
cone_from_subdivision(const Matrix<Scalar>& V_full,
                      const Subdivision& subdivision,
                      const SparseMatrix<Scalar>& restrict_to,
                      SparseMatrix<Scalar>& inequalities,
                      SparseMatrix<Scalar>& equations)
{
   Array<Set<Int>> subdivision_array(subdivision.size());
   auto sa_it = entire(subdivision_array);
   for (auto s_it = entire(subdivision); !s_it.at_end(); ++s_it, ++sa_it)
      *sa_it = Set<Int>(*s_it);
   
   const auto matrices = polytope::secondary_cone_ineq(V_full, subdivision_array, OptionSet());
   inequalities = matrices.first;
   equations    = matrices.second;

   return vertices_from_ineqs(inequalities, equations / restrict_to);
}

template<typename Scalar>
SparseMatrix<Scalar>
find_lineality(const Matrix<Scalar>& V_full,
               const Subdivision& subdivision)
{
   SparseMatrix<Scalar> inequalities, equations;
   SparseMatrix<Scalar> L = cone_from_subdivision(V_full, subdivision, SparseMatrix<Scalar>(0, V_full.rows()), inequalities, equations).second;
   orthogonalize(entire(rows(L)));
   return L;
}
   
#if POLYMAKE_DEBUG

auto
link_of(const Simplex& F, const Subdivision& subdivision)
{
   Set<Simplex> link;  // consider changing this to Subdivision if you use this code for other purposes than check_flip
   for (const auto& sigma: star_of(F, subdivision))
      link += sigma - F;
   return link;
}
   
/*
  Check that the facet really induces a flip on the triangulation.
  For this, each  
  link(sigma cap T_1, T_1)  for  sigma in supp T_Z-
  must be the same as each
  link(sigma cap T_1, T_1)  for  sigma in supp T_Z+
*/
void
check_flip(const Subdivision& T_Z_plus,
           const std::vector<Simplex>& T_Z_minus,
           const Subdivision& subdivision)
{
   Set<Set<Simplex>> links;
   for (const auto& sigma: T_Z_minus)
      links += link_of(sigma, subdivision); 
   if (links.size() > 1) {
      cerr << "secondary_fan: non-unique links for T_Z_minus.\nsubdivision\n" << subdivision
           << "\nT_Z_minus\n" << T_Z_minus
           << "\nT_Z_plus\n" << T_Z_plus
           << "links: " << links << endl;
      throw std::runtime_error("stop");
   }
   for (const auto& sigma: T_Z_plus)
      links += link_of(sigma, subdivision);
   if (links.size() > 1) {
      cerr << "secondary_fan: non-unique links for T_Z_plus.\nsubdivision\n" << subdivision
           << "\nT_Z_minus\n" << T_Z_minus
           << "\nT_Z_plus\n" << T_Z_plus
           << "links: " << links << endl;
      throw std::runtime_error("stop");
   }
}
   
#endif // POLYMAKE_DEBUG

template <typename Scalar>
Set<Int>
facet_indices_among_ineqs(const SparseMatrix<Scalar>& inequalities,
                          const SparseMatrix<Scalar>& rays)
{
   hash_map<Set<Int>, Set<Int>> index_of_zeros;
   Int i = 0;
   FacetList facets;
   for (auto rit = entire(rows(inequalities)); !rit.at_end(); ++rit, ++i) {
      const Set<Int> zeros = orthogonal_rows(rays, *rit);
      facets.insertMax(zeros);
      index_of_zeros[zeros] += i; // for facets, this won't be duplicated; and we don't care about duplication for inequalities
   }
   Set<Int> facet_indices;
   for (auto fit = entire(facets); !fit.at_end(); ++fit)
      facet_indices += index_of_zeros[*fit];
   return facet_indices;
}


template<typename Scalar>
void
flip_from_subdivision(const Subdivision& subdivision,
                      hash_set<Set<Simplex>>& seen_subdivisions,
                      std::list<Subdivision>& active_subdivisions,
                      const Matrix<Scalar>& V_full,
                      const SparseMatrix<Scalar>& ker,
                      hash_map<Vector<Scalar>, Int>& index_of,
                      Int& next_index,
                      FacetList& rays_in_max_cones,
                      const SparseMatrix<Scalar>& restrict_to)
{
   seen_subdivisions += Set<Simplex>(entire(subdivision));
   
   SparseMatrix<Scalar> inequalities, equations;
   const auto rays_and_lin = cone_from_subdivision(V_full, subdivision, restrict_to, inequalities, equations);
   SparseMatrix<Scalar> R = rays_and_lin.first;
   project_to_orthogonal_complement(R, ker);

   rays_in_max_cones.insertMax(indices_of(R, index_of, next_index));
   
   // process the facets of the cone, as they correspond to flips
   const auto facet_indices(facet_indices_among_ineqs(inequalities, R));
   for (Int i : facet_indices) {
      if (restrict_to.rows() &&
          ! polytope::H_input_feasible(inequalities, equations / restrict_to / inequalities[i]))
            continue;

      Simplex neg_part, pos_part;
      for (auto e = entire<indexed>(inequalities[i]); !e.at_end(); ++e) {
         if (*e > Scalar(0))
            pos_part += e.index();
         else
            neg_part += e.index();
      }
      // Calculate the two unique triangulations of the circuit corresponding to the facet
      const auto T_Z_minus = star_of(neg_part, subdivision);
      // we need to ignore the neg_part points to avoid wrong flips
      // on the boundary of the point configuration
      const auto T_Z_plus = join_of(pos_part, boundary_of(T_Z_minus, neg_part));

#if POLYMAKE_DEBUG         
      check_flip(T_Z_plus, T_Z_minus, subdivision);
#endif
         
      Subdivision flipped_subdivision(subdivision);
      for (const auto& sigma: T_Z_minus) flipped_subdivision -= sigma;
      for (const auto& sigma: T_Z_plus)  flipped_subdivision += sigma;

      if (!seen_subdivisions.collect(Set<Simplex>(entire(flipped_subdivision))))
         active_subdivisions.push_back(flipped_subdivision);
   }
}
   
} // end anonymous namespace
   
template<typename Scalar>
BigObject
secondary_fan_impl(const Matrix<Scalar>& V_embed,
                   OptionSet options)
{
   const Matrix<Scalar> V_full = polytope::full_dim_projection(V_embed);
   const Int n = V_embed.rows();

   SparseMatrix<Scalar> restrict_to = options["restrict_to"];
   if (!restrict_to.rows())
      restrict_to = SparseMatrix<Scalar>(0,n);
   
   const Array<Set<Int>> initial_subdivision_array = options["initial_subdivision"];
   Subdivision initial_subdivision;
   for (const auto& s: initial_subdivision_array)
      initial_subdivision += Simplex(s);
   
   if (!initial_subdivision.size()) {
      const RandomSeed seed(options["seed"]);
      initial_subdivision = find_initial_subdivision(V_full, restrict_to, seed);
   }
   
   std::list<Subdivision> active_subdivisions;
   active_subdivisions.emplace_back(initial_subdivision);
   const auto ker(find_lineality(V_full, initial_subdivision));
   
   hash_set<Set<Simplex>> seen_subdivisions;
   FacetList rays_in_max_cones;
   hash_map<Vector<Scalar>, Int> index_of;
   Int next_index = 0;

   while (active_subdivisions.size()) {
      flip_from_subdivision(active_subdivisions.front(), seen_subdivisions, active_subdivisions, V_full, ker, index_of, next_index, rays_in_max_cones, restrict_to);
      active_subdivisions.pop_front();
   }
   
   Matrix<Scalar> ordered_rays(index_of.size(), n);
   for (const auto& index_pair : index_of)
      ordered_rays[index_pair.second] = index_pair.first;

   return BigObject("PolyhedralFan", mlist<Scalar>(),
                    "RAYS", ordered_rays,
                    "MAXIMAL_CONES", IncidenceMatrix<>(rays_in_max_cones.size(), index_of.size(), entire(rays_in_max_cones)),
                    "LINEALITY_SPACE", null_space(ordered_rays / restrict_to));
}

FunctionTemplate4perl("secondary_fan_impl<Scalar>(Matrix<Scalar> { initial_subdivision=>undef, restrict_to=>undef, seed=>undef })");

} }


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
