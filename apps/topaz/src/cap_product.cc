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

#include "polymake/client.h"
#include "polymake/Rational.h"
#include "polymake/Bitset.h"
#include "polymake/SparseVector.h"
#include "polymake/Set.h"
#include "polymake/hash_map"
#include "polymake/vector"
#include "polymake/Map.h"
#include "polymake/topaz/SimplicialComplex_as_FaceMap.h"
#include "polymake/topaz/HomologyComplex.h"
#include <cctype>

namespace polymake { namespace topaz {

namespace {

typedef Set<Int> face_type;
typedef Integer coeff_type;
typedef HomologyComplex< coeff_type, SparseMatrix<coeff_type>, SimplicialComplex_as_FaceMap<Int>> chain_complex;
typedef chain_complex::cycle_type cycle_type;
// cycle_type  CycleGroup: PolymakeStruct coeffs (SparseMatrix<E>) faces (Array<Set<Int>>)

void split_face(face_type& sigma_p, face_type& sigma_q, const face_type& sigma, const Int p)
{
   auto v = entire(sigma);
   sigma_p.clear();
   for (Int i = 0; i < p; ++i, ++v)
      sigma_p.push_back(*v);
   sigma_p.push_back(*v);  // this is the unique vertex contained in both
   sigma_q.clear();
   for ( ; !v.at_end(); ++v)
      sigma_q.push_back(*v);
}
     
} // end anonymous namespace


// computes the cap-product of a cocycle and a cycle
template <typename E, typename VectorType>
Map<Set<Int>, E>
cap_product(const GenericVector<VectorType,E>& co_coeffs, const Array<Set<Int>>& co_faces,
            const GenericVector<VectorType,E>& coeffs,    const Array<Set<Int>>& faces)
{
   Map<Set<Int>, E> cap_product_map;

   if (co_faces.size() == 0 || faces.size() == 0)
      return cap_product_map;

   const Int dim = faces[0].size()-1;
   const Int codim = co_faces[0].size()-1;
   const Int dim_sign = ((dim-codim)*codim)%2 == 0 ? 1 : -1;

   auto sigma = entire(coeffs.top());
   auto sigma_faces = entire(faces);

   face_type sigma_p, sigma_q;

   for ( ; !sigma.at_end(); ++sigma, ++sigma_faces) {
      split_face(sigma_p, sigma_q, *sigma_faces, codim);

      if (cap_product_map.find(sigma_q) == cap_product_map.end())
         cap_product_map[sigma_q] = 0;

      auto f = entire(co_coeffs.top());
      auto f_faces = entire(co_faces);
      for ( ; !f.at_end(); ++f, ++f_faces) {
         if (sigma_p != *f_faces) continue;
         cap_product_map[sigma_q] += dim_sign * (*f) * (*sigma);
      }
   }

   return cap_product_map;
}

// computes the cap-product of a cocycle-group and a cycle-group
template <typename E>
std::pair<CycleGroup<E>, Map<std::pair<Int, Int>, Int>>
cap_product(const CycleGroup<E>& cocycles, const CycleGroup<E>& cycles)
{
   CycleGroup<E> res_group;
   Map<std::pair<Int, Int>, Int> index_map;
   Int count = 0;
   std::vector<Set<Int>> facevector;
   hash_map<Set<Int>, Int> facevector_indices;

   for (auto cocycle = entire<indexed>(rows(cocycles.coeffs)); !cocycle.at_end(); ++cocycle) {
      for (auto cycle = entire<indexed>(rows(cycles.coeffs)); !cycle.at_end(); ++cycle) {
         index_map[std::pair<Int, Int>(cocycle.index(), cycle.index())] = count++;
         const Int rows = res_group.coeffs.rows()+1;
         res_group.coeffs.resize(rows, res_group.coeffs.cols());

         Map<Set<Int>, E> prod = cap_product(*cocycle,cocycles.faces,*cycle,cycles.faces);
         for (auto prod_pair = entire(prod); !prod_pair.at_end(); ++prod_pair) {
            auto facevec_it = facevector_indices.find(prod_pair->first);
            Int col = res_group.coeffs.cols();
            if (facevec_it == facevector_indices.end()) {
               facevector_indices[prod_pair->first] = col;
               facevector.push_back(prod_pair->first);
               res_group.coeffs.resize(rows,col+1);
            } else {
               col = facevec_it->second;
            }
            res_group.coeffs(rows-1,col) = prod_pair->second;
         }
      }
   }

   res_group.faces = Array<Set<Int>>(facevector);
   return { res_group, index_map };
}

UserFunctionTemplate4perl("# @category Topology"
                          "# Compute all cap products of cohomology and homology cycles in two given groups."
                          "# @param CycleGroup<E> cocycles"
                          "# @param CycleGroup<E> cycles"
                          "# @return Pair<CycleGroup<E>,Map<Pair<Int,Int>,Int>>"
                          "# @example The following stores all cap products of the cocycles and cycles"
                          "#  of the homology group of the oriented surface of genus 1 in the variable $cp."
                          "# > $s = surface(1);"
                          "# > $cp = cap_product($s->COCYCLES->[1],$s->CYCLES->[1]);",
                          "cap_product<E>(CycleGroup<E> CycleGroup<E>)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
