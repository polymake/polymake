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
#include "polymake/polytope/matroid_polytopes.h"
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/Array.h"
#include "polymake/PowerSet.h"
#include "polymake/internal/PlainParser.h"
#include "polymake/SparseMatrix.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Polynomial.h"

namespace polymake { namespace polytope {

BigObject hypersimplex(Int k, Int d, OptionSet options)
{
   if (d < 2)
      throw std::runtime_error("hypersimplex: dimension >= 2 required");
   if (k <= 0 || k >= d)
      throw std::runtime_error("hypersimplex: 0 < k < d required");

   BigObject p("Polytope<Rational>");
   p.set_description() << "(" << k << "," << d << ")-hypersimplex" << endl;

   p.take("CONE_AMBIENT_DIM") << d+1;
   p.take("CONE_DIM") << d;
   p.take("BOUNDED") << true;

   // we already know the number of vertices
   const Int n(Integer::binom(d,k));
   p.take("N_VERTICES") << n;

   const bool group_flag = options["group"];
   const bool nov_flag = options["no_vertices"];
   bool nof_flag = options["no_facets"];
   bool novif_flag = options["no_vif"];

   // if the hypersimplex degenerates to a simplex the formula for the facets below don't work
   if (k == 1 || k == d-1) {
      nof_flag = novif_flag = true;
   }

   if (!nov_flag) {
      std::ostringstream label;
      Array<std::string> labels(n);
      auto label_it = labels.begin();
      Matrix<Rational> Vertices(n,d+1);
      auto v = rows(Vertices).begin();
      for (auto s=entire(all_subsets_of_k(range(0,d-1),k)); !s.at_end(); ++s, ++v, ++label_it) {
         (*v)[0]=1;
         v->slice(range_from(1)).slice(*s).fill(1);
         label.str("");
         wrap(label) << *s;
         *label_it = label.str();
      }
      p.take("VERTEX_LABELS") << labels;
      p.take("VERTICES") << Vertices;

      if (!novif_flag) {
         IncidenceMatrix<> VIF(2*d,n);
         for (Int i = 0; i < d; ++i) {
            VIF.row(2*i)  = indices(attach_selector(Vertices.col(i+1), operations::non_zero()));
            VIF.row(2*i+1)= indices(attach_selector(Vertices.col(i+1), operations::is_zero()));
         }
         p.take("VERTICES_IN_FACETS") << VIF;
      }
   }

   if (nov_flag && !novif_flag) {
      IncidenceMatrix<> VIF(2*d,n);
      Int j = 0;
      for (auto s = entire(all_subsets_of_k(range(0,d-1), k)); !s.at_end(); ++s, ++j) {
         for (Int i = 0; i < d; ++i) {
            if (Set<Int>(*s).contains(i)) {
               VIF.row(2*i) += j;
            } else {
               VIF.row(2*i+1) += j;
            }
         }
      }
      p.take("VERTICES_IN_FACETS") << VIF;
   }


   // the facets are not unique
   if ( !nof_flag ) {
      SparseMatrix<Rational> F(2*d,d+1);
      Rows< SparseMatrix<Rational> >::iterator f=rows(F).begin();
      for (Int i = 1; i <= d; ++i) { // Facet 2*i and Facet 2*i+1 are parallel
         (*f)[0]=1;
         (*f)[i]=-1;
         ++f;
         (*f)[0]=0;
         (*f)[i]=1;
         ++f;
      }
      Matrix<Rational> A(1,d+1);
      A.row(0)=ones_vector<Rational>(d+1);
      A.row(0)[0]=-k;
      p.take("FACETS") << F;
      p.take("AFFINE_HULL") << A;
   }
   
   if(!nof_flag || !nov_flag)
	   p.take("EHRHART_POLYNOMIAL") << ehrhart_polynomial_hypersimplex(k,d);

   // generate the combinatorial symmetry group on the vertices
   if (group_flag) {
      Array<Array<Int>> gens(2);
      Array<Int> gen{sequence(0,d)};
      gen[0] = 1;
      gen[1] = 0;
      gens[0] = gen;

      gen[0] = d-1;
      for (Int j = 1; j <= d-1; ++j) {
         gen[j] = j-1;
      }
      gens[1] = gen;

      BigObject a("group::PermutationAction", "GENERATORS", gens);
      BigObject g("group::Group", "fullCombinatorialGroupOnCoords");
      g.set_description() << "full combinatorial group on coordinates of " <<  "(" << k << "," << d << ")-hypersimplex" << endl;
      p.take("GROUP") << g;
      p.take("GROUP.COORDINATE_ACTION") << a;
   }
   
   return p;
}

Set<Int> matroid_indices_of_hypersimplex_vertices(BigObject m)
{
   const Array<Set<Int>> bases=m.give("BASES");
   const Int n = m.give("N_ELEMENTS");
   const Int d = m.give("RANK");
   Set<Int> set;
   for (const auto& b : bases) {
      Int sum = 0;
      Int temp_d = d;
      Int temp = 0;
      for (const auto& i : b) {
         if (temp_d == d && i != 0)
            sum += Int(Integer::binom(n-1, d-1));
         --temp_d;
         for (Int k = 1; k <= i-temp-1; ++k)
            sum += Int(Integer::binom(n-temp-1-k, temp_d));
         temp=i;
      }
      set += sum;
   }
   return set;
}

UserFunction4perl("# @category Producing a polytope from scratch"
                  "# Produce the hypersimplex $ Δ(k,d) $, that is the the convex hull of all 0/1-vector in $ R^d $"
                  "# with exactly //k// 1s."
                  "# Note that the output is never full-dimensional."
                  "# @param Int k number of 1s"
                  "# @param Int d ambient dimension"
                  "# @option Bool group"
                  "# @option Bool no_vertices do not compute vertices"
                  "# @option Bool no_facets do not compute facets"
                  "# @option Bool no_vif do not compute vertices in facets"
                  "# @return Polytope"
                  "# @example This creates the hypersimplex in dimension 4 with vertices with exactly two 1-entries"
                  "# and computes its symmetry group:"
                  "# > $h = hypersimplex(2,4,group=>1);",
                  &hypersimplex, "hypersimplex($,$;{group=>undef, no_vertices=>0, no_facets=>0, no_vif=>0 })");

InsertEmbeddedRule("REQUIRE_APPLICATION matroid\n\n");

UserFunction4perl("# @category Other"
                  "# For a given matroid return the bases as a"
                  "# subset of the vertices of the hypersimplex"
                  "# @option matroid::Matroid m the matroid"
                  "# @return Set<Int>",
                  &matroid_indices_of_hypersimplex_vertices, "matroid_indices_of_hypersimplex_vertices(matroid::Matroid)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
