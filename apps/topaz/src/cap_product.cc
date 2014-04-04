/* Copyright (c) 1997-2014
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
#include "polymake/topaz/SimplicialComplex_as_FaceMap.h"
#include "polymake/topaz/ChainComplex.h"
#include <cctype>

namespace polymake { namespace topaz {

namespace {

typedef Set<int> face_type;
typedef Integer coeff_type;
typedef ChainComplex< coeff_type, SimplicialComplex_as_FaceMap<int> > chain_complex;
typedef chain_complex::cycle_type cycle_type;

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

void cap_product(perl::Object p)
{
   const  Array<cycle_type> &CoCycles=p.give("COCYCLES");
   const  Array<cycle_type> &Cycles=p.give("CYCLES");

   
   const int d(CoCycles.size()-1);
   face_type sigma_p, sigma_q;
   
   for (int p=0; p<=d; ++p) {
      const cycle_type::coeff_matrix co_signs(CoCycles[p].coeffs);
      if (co_signs.rows() == 0) continue;
      const cycle_type::face_list co_faces(CoCycles[p].faces);
      for (int r=p; r<=d; ++r) {
         const cycle_type::coeff_matrix signs(Cycles[r].coeffs);
         if (signs.rows() == 0) continue;
         const cycle_type::face_list faces(Cycles[r].faces);
         
         cout << "[H^" << p << " cap H_" << r << "]\n";
         const int q(r-p);
         const int pq_sign((p*q)%2==0? 1 : -1);
         
         for (int i=0; i<co_signs.rows(); ++i)
            for (int k=0; k<signs.rows(); ++k) {
               cout << i << "*" << k << ": ";
               
               Entire<cycle_type::coeff_matrix::row_type>::const_iterator sigma(entire(signs[k]));
               Entire<cycle_type::face_list>::const_iterator sigma_faces(entire(faces));
               
               hash_map<Bitset,coeff_type> cap_product_vector;
               
               for ( ; !sigma.at_end(); ++sigma, ++sigma_faces) {
                  split_face(sigma_p,sigma_q,*sigma_faces,p);
                  Bitset sigma_q_bitset(sigma_q);
                  if (cap_product_vector.find(sigma_q_bitset)==cap_product_vector.end())
                     cap_product_vector[sigma_q_bitset]=0;
                  
                  Entire<cycle_type::coeff_matrix::row_type>::const_iterator f(entire(co_signs[i]));
                  Entire<cycle_type::face_list>::const_iterator f_faces(entire(co_faces));
                  for ( ; !f.at_end(); ++f, ++f_faces) {
                     if (sigma_p != *f_faces) continue;
                     cap_product_vector[sigma_q_bitset]+=pq_sign*(*f)*(*sigma);
                  }
               }
               bool first_term(true);
               coeff_type sum(0);
               for (Entire< hash_map<Bitset,coeff_type> >::const_iterator c(entire(cap_product_vector)); !c.at_end(); ++c) {
                  sum += c->second;
                  const int sgn=sign(c->second);
                  if (sgn) {
                     const coeff_type val(abs(c->second));
                     if (sgn>0) {
                        if (!first_term)
                           cout << " + ";
                     } else
                        cout << " - ";
                     if (val!=1) cout << val << " ";
                     cout << c->first;
                     first_term=false;
                  }
                  
               }
               cout << " (" << sum << ")\n";
            }
         cout << endl;
      }
   }
}

UserFunction4perl("#Compute and print all cap products of cohomology and homology cycles."
                  "#args: SimplicialComplex",
                  &cap_product,"cap_product(SimplicialComplex)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
