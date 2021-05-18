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
#include "polymake/Matrix.h"
#include "polymake/Integer.h"
#include "polymake/PowerSet.h"
#include "polymake/linalg.h"
#include "polymake/list"
#include "polymake/group/permlib.h"
#include "polymake/permutations.h"


namespace polymake { namespace matroid {

/*
  normalize vec, s.t. the last non zero componet is 1
  and calculates the (unique) non-negative integer v_0 + v_1 *p + v_2 *p^2 -cor < p^2+p+1
  where cor is a correction for omitet numbers, since some vectors in projectiv space are eqivalent.
*/
Int vector_to_int(const Vector<Int>& vec, const Int p)
{
   Int cor = 1;
   Int factor = 1;
   if (vec[2] % p != 0) {
      cor = p*p-p-1;         //=(p-2)*p+(p-2)+1
      ExtGCD<Int> gcd = ext_gcd(vec[2], p);
      factor = gcd.p; // * vec[2] = 1 mod p
   } else if (vec[1] % p !=0 ) {
      cor = p-1;
      ExtGCD<Int> gcd = ext_gcd(vec[1], p);
      factor = gcd.p; // * vec[1] = 1 mod p
   } else {
      ExtGCD<Int> gcd = ext_gcd(vec[0], p);
      factor = gcd.p; // * vec[0] = 1 mod p
   }
   if (factor < 0) {
      factor = (factor%p)+p;
   }
   return (factor*vec[0])%p + p*((factor*vec[1])%p) +  p*p*((factor*vec[2])%p) - cor;
}

BigObject projective_plane(const Int p)
{
   if (p < 1) {
      throw std::runtime_error("projective_plane: p should be positive.");	
   }
   const Int n_elements = p*p+p+1;
   const Int rank = 3;

   // generators for SL_3(Z) (this is (mod p) not SL_3(F_p)) but sufficient.
   Matrix<Int> m0(3,3);
   m0(0, 1)=1;                         // 0 1 0
   m0(1, 2)=1;                         // 0 0 1
   m0(2, 0)=1;                         // 1 0 0

   Matrix<Int> m1(3,3);
   m1(0, 0)=1;                         // 1  0  1
   m1(0, 2)=1;                         // 0 -1 -1
   m1(1, 1)=p-1;                       // 0  1  0
   m1(1, 2)=p-1;
   m1(2, 1)=1;

   Matrix<Int> m2(3,3);
   m2(0, 1)=1;                         // 0  1  0
   m2(1, 0)=1;                         // 1  0  0
   m2(2, 0)=p-1;                       //-1 -1 -1
   m2(2, 1)=p-1;
   m2(2, 2)=p-1;

   Array<Int> gen0(n_elements);
   Array<Int> gen1(n_elements);
   Array<Int> gen2(n_elements);

   //action of the generators as permutation:
   Vector<Int> vec(3);
   for (Int j = 0; j < n_elements; ++j) {     // iteration over rep. of P(GF_p)^2
     Int k = p*p-p-1+j;
     if (j == 0) {
       k = 1;
     } else if (j < p+1) {
        k = p-1+j;
     }
     vec[0] = k%p;
     vec[1] = k/p %p;
     vec[2] = k/(p*p) %p;             //vec is normalized, such that the last non zero component is 1
     gen0[j] = vector_to_int(m0*vec,p);
     gen1[j] = vector_to_int(m1*vec,p);
     gen2[j] = vector_to_int(m2*vec,p);
   }

   Array<Array<Int>> group{ gen0, gen1, gen2 };

   for (Int i = 0; i < 3; ++i)
      if (!pm::is_permutation(group[i])) {
         throw std::runtime_error("projective_plane: p should be a prime.");
      }

   const group::PermlibGroup sym_group(group);
   Set<Int> basis{0L, 1L, p+1};
   Set<Set<Int>> bases{ sym_group.orbit(basis) };

   BigObject m("Matroid",
               "BASES", bases,
               "RANK", rank,
               "N_ELEMENTS", n_elements);
   if (p == 2) {
      m.set_description()<<"The Fano matroid, also known as the 7-point projective plane of order 2, which is representable if and only if the corresponding Field has characteristic 2."<<endl;
   } else {
      m.set_description()<<"The "<<n_elements<<"-point projective plane of order "<<p<<", which is representable if and only if the corresponding Field has characteristic "<<p<<"."<<endl;
   }
   return m;
}


UserFunction4perl("# @category Producing a matroid from scratch\n"
                  "# Creates the projective plane matroid of rank 3 with //p^2+p+1// elements, where p is a prime."
                  "# @param Integer p"
                  "# @return Matroid",
                  &projective_plane, "projective_plane");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
 
