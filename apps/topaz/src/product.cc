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
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/PowerSet.h"
#include "polymake/Array.h"
#include "polymake/Rational.h"
#include "polymake/Map.h"
#include "polymake/Graph.h"
#include "polymake/list"
#include <string>

namespace polymake { namespace topaz {
namespace {

// m_set is an m-element set of {0,...,m+n-1}
std::vector<bool> shuffle_from_m_set(const Int m, const Int n, const Set<Int>& m_set)
{
   std::vector<bool> shuffle(m+n-2, false);
   for (const Int i : m_set)
      shuffle[i] = true;
   return shuffle;
}

// m_set is an m-element set of {0,...,m+n-1}
Set<Int> facet_from_m_set(const std::list<Int>& s1, const std::list<Int>& s2,
                          const Matrix<Int>& vert_map, const Set<Int>& m_set)
{
   auto v1 = s1.begin();
   auto v2 = s2.begin();
   Set<Int> facet;
   facet += vert_map(*v1, *v2);
   //      cout << *v1 << "," << *v2 << " -- " << vert_map(*v1,*v2) << endl;
   const std::vector<bool> shuffle = shuffle_from_m_set(s1.size(), s2.size(), m_set);
   for (bool s : shuffle) {
      if (s)  ++v1;
      else    ++v2;
      facet += vert_map(*v1, *v2);
   }

   return facet;
}
   

Array<Int> color_cons_ordering(const Array<Int>& C)
{
   Array<Int> ordering(C.size());

   Set<Int> colors;
   Map<Int, std::list<Int>> vert_of_color;
   for (Int i = 0; i < C.size(); ++i) {
      colors += C[i];
      vert_of_color[ C[i] ].push_back(i);
   }

   Int i = 0;
   for (auto c = entire(colors); !c.at_end(); ++c)
      for (auto v = entire(vert_of_color[*c]); !v.at_end(); ++v, ++i)
         ordering[i] = *v;

   return ordering;
}
   
void combinatorial_simplicial_product_impl (BigObject p_in1, BigObject p_in2, BigObject& p_out, Array<Int>& order1, Array<Int>& order2, OptionSet options)
{
   const bool no_labels = options["no_labels"];
   const Array<Set<Int>> C1 = p_in1.give("FACETS");
   const Array<Set<Int>> C2 = p_in2.give("FACETS");
   const Int n_vert1 = p_in1.give("N_VERTICES");
   const Int n_vert2 = p_in2.give("N_VERTICES");

   // read orderings or set default
   order1 = Array<Int>(n_vert1);
   order2 = Array<Int>(n_vert2);
   
   if (!(options["vertex_order1"] >> order1)) {
      if (options["color_cons"]) {
         const bool is_foldable = p_in1.give("FOLDABLE");
         if (!is_foldable)
            throw std::runtime_error("simplicial_product: Complex 1 is not FOLDABLE");
         const Array<Int> coloring = p_in1.give("COLORING");
         order1 = color_cons_ordering(coloring);
      } else {
         for (Int i = 0; i < n_vert1; ++i)
            order1[i] = i;
      }
   }

   if (!(options["vertex_order2"] >> order2)) {
      if (options["color_cons"]) {
         const bool is_foldable = p_in2.give("FOLDABLE");
         if (!is_foldable)
            throw std::runtime_error("simplicial_product: Complex 2 is not FOLDABLE");
         const Array<Int> coloring = p_in2.give("COLORING");
         order2 = color_cons_ordering(coloring);
      } else {
         for (Int i = 0; i < n_vert2; ++i)
            order2[i] = i;
      }
   }

   // compute vertex map
   Matrix<Int> vert_map(n_vert1, n_vert2);
   Int c = 0;
   for (auto v2 = entire(order2); !v2.at_end(); ++v2)
      for (auto v1=entire(order1); !v1.at_end(); ++v1, ++c)
         vert_map(*v1,*v2) = c;
   
   std::list<Set<Int>> F;
   for (auto f1 = entire(C1); !f1.at_end(); ++f1)
      for (auto f2 = entire(C2); !f2.at_end(); ++f2) {
         const Int m = f1->size()-1;
         const Int n = f2->size()-1;

         std::list<Int> s1, s2;
         for (Int i = 0; i < n_vert1; ++i)
            if (f1->contains(order1[i]))
               s1.push_back(order1[i]);
         for (Int i = 0; i < n_vert2; ++i)
            if (f2->contains(order2[i]))
               s2.push_back(order2[i]);

         // compute faces
         for (auto m_set=entire(all_subsets_of_k(range(0,m+n-1), m)); !m_set.at_end(); ++m_set)
            F.push_back(facet_from_m_set(s1, s2, vert_map, *m_set));
      }
   
   p_out.set_description() << "Simplicial product of " << p_in1.name() << " and " << p_in2.name() << "."<<endl;
   p_out.take("FACETS") << as_array(F);

   if (!no_labels) {
      const Array<std::string> L1 = p_in1.give("VERTEX_LABELS");
      const Array<std::string> L2 = p_in2.give("VERTEX_LABELS");
      Array<std::string> L(L1.size()*L2.size());
      
      c=0;
      for (auto v2=entire(order2); !v2.at_end(); ++v2)
         for (auto v1=entire(order1); !v1.at_end(); ++v1, ++c)
            L[c] = '(' + L1[*v1] + ',' + L2[*v2] + ')';

      p_out.take("VERTEX_LABELS") << L;
   }
}

BigObject combinatorial_simplicial_product (BigObject p_in1, BigObject p_in2, OptionSet options)
{
   BigObject p_out("SimplicialComplex");
   Array<Int> order1, order2;
   combinatorial_simplicial_product_impl(p_in1, p_in2, p_out, order1, order2, options);
   return p_out;
}

template <typename Scalar>
BigObject simplicial_product (BigObject p_in1, BigObject p_in2, OptionSet options)
{
   const bool realize = options["geometric_realization"];
   BigObjectType result_type = realize
      ? BigObjectType("GeometricSimplicialComplex", mlist<Scalar>())
      : BigObjectType("SimplicialComplex");
   BigObject p_out(result_type);
   Array<Int> order1, order2;
   combinatorial_simplicial_product_impl(p_in1, p_in2, p_out, order1, order2, options);
   
   if (realize) {
      const Matrix<Scalar> GR1 = p_in1.give("COORDINATES");
      const Matrix<Scalar> GR2 = p_in2.give("COORDINATES");
      Matrix<Scalar> GR(GR1.rows()*GR2.rows(),GR1.cols()+GR2.cols());
      Int c = 0;
      for (auto v2=entire(order2); !v2.at_end(); ++v2)
         for (auto v1=entire(order1); !v1.at_end(); ++v1, ++c)
            GR[c] = GR1[*v1] | GR2[*v2];
      
      p_out.take("COORDINATES") << GR;
   }

   return p_out;
}

} // end anonymous namespace

UserFunction4perl("# @category Producing a new simplicial complex from others\n"
                  "# Computes the __simplicial product__ of two complexes.\n"
                  "# Vertex orderings may be given as options.\n"
                  "# @param SimplicialComplex complex1"
                  "# @param SimplicialComplex complex2"
                  "# @option Array<Int> vertex_order1"
                  "# @option Array<Int> vertex_order2"
                  "# @option Bool geometric_realization default 0"
                  "# @option Bool color_cons"
                  "# @option Bool no_labels Do not create [[VERTEX_LABELS]]. default: 0"
                  "# @return SimplicialComplex",
                  &combinatorial_simplicial_product, "simplicial_product(SimplicialComplex, SimplicialComplex, {vertex_order1 => undef, vertex_order2 => undef, geometric_realization => 0, color_cons => 0, no_labels => 0})");

UserFunctionTemplate4perl("# @category Producing a new simplicial complex from others\n"
                  "# Computes the __simplicial product__ of two complexes.\n"
                  "# Vertex orderings may be given as options.\n"
                  "# @param GeometricSimplicialComplex complex1"
                  "# @param GeometricSimplicialComplex complex2"
                  "# @tparam Scalar"
                  "# @option Array<Int> vertex_order1"
                  "# @option Array<Int> vertex_order2"
                  "# @option Bool geometric_realization default 1"
                  "# @option Bool color_cons"
                  "# @option Bool no_labels Do not create [[VERTEX_LABELS]]. default: 0"
                  "# @return GeometricSimplicialComplex<Scalar>",
                  "simplicial_product<Scalar>(GeometricSimplicialComplex<Scalar>, GeometricSimplicialComplex<Scalar>, {vertex_order1 => undef, vertex_order2 => undef, geometric_realization => 1, color_cons => 0, no_labels => 0})");

   
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
