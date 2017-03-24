/* Copyright (c) 1997-2015
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

typedef Set<int> face_type;
typedef Integer coeff_type;
typedef HomologyComplex< coeff_type, SparseMatrix<coeff_type>, SimplicialComplex_as_FaceMap<int> > chain_complex;
typedef chain_complex::cycle_type cycle_type;
// cycle_type  CycleGroup: PolymakeStruct coeffs (SparseMatrix<E>) faces (Array<Set<int>>)

void split_face(face_type& sigma_p, face_type& sigma_q, const face_type& sigma, const int p)
{
   Entire< Set<int> >::const_iterator v(entire(sigma));
   sigma_p.clear();
   for (int i=0; i<p; ++i, ++v)
      sigma_p.push_back(*v);
   sigma_p.push_back(*v);  // this is the unique vertex contained in both
   sigma_q.clear();
   for ( ; !v.at_end(); ++v)
      sigma_q.push_back(*v);
}
     
} // end anonymous namespace


// computes the cap-product of a cocycle and a cycle
template <typename E, typename VectorType>
Map< Set<int>, E >
cap_product(const GenericVector<VectorType,E>& co_coeffs, const Array< Set<int> >& co_faces,
            const GenericVector<VectorType,E>& coeffs,    const Array< Set<int> >& faces)
{
   Map< Set<int>, E > cap_product_map;

   if (co_faces.size() == 0 || faces.size() == 0)
      return cap_product_map;

   int dim = faces[0].size()-1;
   int codim = co_faces[0].size()-1;
   const int dim_sign = ((dim-codim)*codim)%2 == 0 ? 1 : -1;

   typename Entire< VectorType >::const_iterator sigma = entire(coeffs.top());
   Entire< Array< Set<int> > >::const_iterator sigma_faces(entire(faces));

   face_type sigma_p, sigma_q;

   for ( ; !sigma.at_end(); ++sigma, ++sigma_faces) {
      split_face(sigma_p, sigma_q, *sigma_faces, codim);

      if (cap_product_map.find(sigma_q) == cap_product_map.end())
         cap_product_map[sigma_q] = 0;

      typename Entire< VectorType >::const_iterator f = entire(co_coeffs.top());
      Entire< Array< Set<int> > >::const_iterator f_faces(entire(co_faces));
      for ( ; !f.at_end(); ++f, ++f_faces) {
         if (sigma_p != *f_faces) continue;
         cap_product_map[sigma_q] += dim_sign * (*f) * (*sigma);
      }
   }

   return cap_product_map;
}

// computes the cap-product of a cocycle-group and a cycle-group
template <typename E>
std::pair< CycleGroup<E> , Map< std::pair<int,int>, int > >
cap_product(const CycleGroup<E>& cocycles, const CycleGroup<E>& cycles)
{
   CycleGroup<E> res_group;
   Map< std::pair<int,int>, int> index_map;
   int count = 0;
   std::vector< Set<int> > facevector;
   hash_map< Set<int>, int > facevector_indices;

   for(typename pm::ensure_features<Rows<typename CycleGroup<E>::coeff_matrix>, pm::cons<pm::end_sensitive,pm::indexed> >::const_iterator cocycle = ensure(rows(cocycles.coeffs), (pm::cons<pm::end_sensitive, pm::indexed>*)0).begin();
         !cocycle.at_end(); ++cocycle)
   {
      for(typename pm::ensure_features<Rows<typename CycleGroup<E>::coeff_matrix>, pm::cons<pm::end_sensitive,pm::indexed> >::const_iterator cycle = ensure(rows(cycles.coeffs), (pm::cons<pm::end_sensitive, pm::indexed>*)0).begin();
            !cycle.at_end(); ++cycle)
      {
         index_map[std::pair<int,int>(cocycle.index(), cycle.index())] = count++;
         const int rows = res_group.coeffs.rows()+1;
         res_group.coeffs.resize(rows, res_group.coeffs.cols());

         Map< Set<int>, E > prod(cap_product(*cocycle,cocycles.faces,*cycle,cycles.faces));
         for(typename Entire< Map< Set<int>, E > >::const_iterator prod_pair = entire(prod); !prod_pair.at_end(); ++prod_pair)
         {
            hash_map< Set<int>, int >::iterator facevec_it = facevector_indices.find(prod_pair->first);
            int col = res_group.coeffs.cols();
            if (facevec_it == facevector_indices.end())
            {
               facevector_indices[prod_pair->first] = col;
               facevector.push_back(prod_pair->first);
               res_group.coeffs.resize(rows,col+1);
            }
            else
            {
               col = facevec_it->second;
            }
            res_group.coeffs(rows-1,col) = prod_pair->second;
         }
      }
   }

   res_group.faces = Array< Set<int> >(facevector);
   return std::make_pair(res_group,index_map);
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
