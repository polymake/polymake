/* Copyright (c) 1997-2018
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
#include "polymake/Set.h"
#include "polymake/hash_map"
#include "polymake/Vector.h"
#include "polymake/Matrix.h"
#include "polymake/topaz/SimplicialComplex_as_FaceMap.h"
#include "polymake/topaz/HomologyComplex.h"
#include "polymake/topaz/IntersectionForm.h"

namespace polymake { namespace topaz {

namespace {

typedef Rational coeff_type;
typedef Matrix<coeff_type> cup_type;
typedef Array<int> reordering_type;

bool pivot(int& k, const int i, const cup_type& M, const reordering_type& ind)
{
   const int n(M.rows());
   k=i;
   while (k<n && is_zero(M(ind[k],ind[k])))
      ++k;
   return (k<n);
}

bool non_zero(int& k, const int i, const cup_type& M, const reordering_type& ind)
{
   const int n(M.rows());
   k=i;
   while (k<n && is_zero(M(ind[k],ind[i])))
      ++k;
   return (k<n);
}

void signature(cup_type& M, int& positive, int& negative)
{
   const int n=M.rows();
   reordering_type ind(n,entire(sequence(0,n)));

   int i(0), k;
   positive=negative=0;

   while (i<n) {
      if(pivot(k,i,M,ind)) {
         // eliminate with pivot element on the diagonal
         if (k!=i) std::swap(ind[i],ind[k]);
         coeff_type p(M(ind[i],ind[i]));
         
         for (int j=i+1; j<n; ++j) {
            coeff_type c(M(ind[j],ind[i])/p);
            M[ind[j]]-=c*M[ind[i]];
         }
         for (int j=i+1; j<n; ++j)
            M(ind[i],ind[j])=0;

         if (p>0)
            ++positive;
         else
            ++negative;

         ++i;
      } else {
         // diagonalize hyperbolic pairs
         if (non_zero(k,i,M,ind)) { // i!=k
            if (k!=i+1) std::swap(ind[i+1],ind[k]);

            Vector<coeff_type> sum(M[ind[i]]+M[ind[i+1]]), diff(M[ind[i]]-M[ind[i+1]]);
            M[ind[i]]=sum; M.col(ind[i])=sum; M[ind[i+1]]=diff; M.col(ind[i+1])=diff;
            M(ind[i],ind[i])*=2; M(ind[i+1],ind[i+1])*=-2;
         } else   // zero row/column
            ++i;
         
      }
   }
}

} // end unnamed namespace

void intersection_form(perl::Object p)
{
   typedef CycleGroup<Integer> cycle_type;
   const Array<cycle_type> Cycles = p.give("CYCLES");

   const int d(Cycles.size()-1);
   if (d%4 != 0)
      throw std::runtime_error("intersection_form: Dimension " + std::to_string(d) + " not divisible by 4");

   const cycle_type::face_list facets(Cycles[d].faces);
   const cycle_type::coeff_matrix signs(Cycles[d].coeffs);

   if (signs.rows() != 1)
      throw std::runtime_error("intersection_form: Expected exactly one top level homology class, found " + std::to_string(signs.rows()));

   auto f = entire(facets);
   auto s = entire(signs[0]);
   hash_map<Bitset,Integer> SignedFacets;
   for ( ; !f.at_end() && !s.at_end(); ++f, ++s)
      SignedFacets[Bitset(*f)]=*s;

   const Array<cycle_type> CoCycles = p.give("COCYCLES");
   const cycle_type::face_list small_faces(CoCycles[d/2].faces);
   const cycle_type::coeff_matrix small_cocycles(CoCycles[d/2].coeffs);
   const int n(small_cocycles.rows());

   cup_type Cup(n,n);

   int parity=0; // until we are not convinced of the converse we assume that the intersection form is even

   for (auto c1 = entire<indexed>(rows(small_cocycles)); !c1.at_end(); ++c1) {
      for (auto c2 = entire<indexed>(rows(small_cocycles)); !c2.at_end(); ++c2) {
         Integer cup_product(0);
         for (auto x = entire(*c1); !x.at_end(); ++x) {
            const Set<int> face_x(small_faces[x.index()]);
            const Bitset bit_face_x(face_x);
            for (auto y = entire(*c2); !y.at_end(); ++y) {
               const Set<int> face_y(small_faces[y.index()]);
               Bitset this_union(bit_face_x+Bitset(face_y));
               if (face_x.back()==face_y.front() && SignedFacets.find(this_union)!=SignedFacets.end())
                  cup_product+=SignedFacets[this_union]*(*x)*(*y);
            }
         }
         Cup(c1.index(), c2.index())=cup_product;
         if (c1.index() == c2.index() && cup_product.odd())
            parity=1;
      }
   }
   
#if POLYMAKE_DEBUG
   for (int i=0; i<n; ++i)
      for (int j=0; j<i; ++j)
         if (Cup(i,j) != Cup(j,i)) {
            std::ostringstream err;
            err << "resulting cup product matrix not symmetric: [" << i << "," << j << "]\n";
            wrap(err) << Cup;
            throw std::runtime_error(err.str());
         }
#endif

   IntersectionForm IF;
   IF.parity=parity;
   signature(Cup,IF.positive,IF.negative);
   p.take("INTERSECTION_FORM") << IF;
}

Function4perl(&intersection_form, "intersection_form(SimplicialComplex)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
