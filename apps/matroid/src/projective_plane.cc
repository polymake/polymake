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
#include "polymake/Matrix.h"
#include "polymake/Integer.h"
#include "polymake/PowerSet.h"
#include "polymake/linalg.h"
#include "polymake/list"
#include "polymake/group/permlib.h"
#include "polymake/permutations.h"


namespace polymake { namespace matroid {

/*
normalizs vec, s.t. the last non zero componet is 1
and calculates the (unique) non-negative integer v_0 + v_1 *p + v_2 *p^2 -cor < p^2+p+1
where cor is a correction for omitet numbers, since some vectors in projectiv space are eqivalent.
*/ 
int vector_to_int(const Vector<int> vec, const int p){
   int cor=1;
   int factor=1;
   if(vec[2]%p!=0){
      cor=p*p-p-1;         //=(p-2)*p+(p-2)+1
      ExtGCD<long int> gcd=ext_gcd(vec[2],p);
      factor=(int) gcd.p; // * vec[2] = 1 mod p
   }else if(vec[1]%p!=0){
      cor=p-1;
      ExtGCD<long int> gcd=ext_gcd(vec[1],p);
      factor=(int) gcd.p; // * vec[1] = 1 mod p
   }else{
      ExtGCD<long int> gcd=ext_gcd(vec[0],p);
      factor=(int) gcd.p; // * vec[0] = 1 mod p
   }
   if(factor<0){
      factor=(factor%p)+p;
   }
   return(factor*vec[0])%p + p*((factor*vec[1])%p) +  p*p*((factor*vec[2])%p) -cor;
}

perl::Object projective_plane(const int p){
   if(p<1){
	throw std::runtime_error("projective_plane: p should be positive.");	
   }
   perl::Object m("Matroid");
   int  n_elements=p*p+p+1;
   int rank=3;

   //generators for SL_3(Z) (this is (mod p) not SL_3(F_p)) but sufficient.
   Matrix<int> m0(3,3);
   m0[0][1]=1;                         // 0 1 0
   m0[1][2]=1;                         // 0 0 1
   m0[2][0]=1;                         // 1 0 0

   Matrix<int> m1(3,3);
   m1[0][0]=1;                         // 1  0  1
   m1[0][2]=1;                         // 0 -1 -1
   m1[1][1]=p-1;                       // 0  1  0
   m1[1][2]=p-1;
   m1[2][1]=1;

   Matrix<int> m2(3,3);
   m2[0][1]=1;                         // 0  1  0
   m2[1][0]=1;                         // 1  0  0
   m2[2][0]=p-1;                       //-1 -1 -1
   m2[2][1]=p-1;
   m2[2][2]=p-1;

   Array<int> gen0(n_elements);
   Array<int> gen1(n_elements);
   Array<int> gen2(n_elements);

   //action of the generators as permutation:
   Vector<int> vec(3);
   for(int j=0;j<n_elements;++j){     // iteration over rep. of P(GF_p)^2
     int k=p*p-p-1+j;
     if(j==0){
       k=1;
     }else{
       if(j<p+1){
	 k=p-1+j;
       }
     }
     vec[0]=k %p;
     vec[1]=(k)/p %p;
     vec[2]=(k)/(p*p) %p;             //vec is normalized, such that the last non zero componet is 1
     gen0[j]=vector_to_int(m0*vec,p);
     gen1[j]=vector_to_int(m1*vec,p);
     gen2[j]=vector_to_int(m2*vec,p);
   }

   Array< Array<int> > group(3);
   group[0]=gen0;
   group[1]=gen1;
   group[2]=gen2;

   for(int i=0;i<3;++i)
      if(!pm::is_permutation(group[i])){
         throw std::runtime_error("projective_plane: p should be a prime.");
      }

   const group::PermlibGroup sym_group(group);
   Set<int> basis{0, 1, p+1};
   Set<Set<int>> bases{ sym_group.orbit(basis) };

   m.take("BASES") << bases;
   m.take("RANK") << rank;
   m.take("N_ELEMENTS") << n_elements;
   if(p==2){
      m.set_description()<<"The Fano matroid, also known as the 7-point projective plane of order 2, which is representable if and only if the corresponding Field has characteristic 2."<<endl;
   }else{
      m.set_description()<<"The "<<n_elements<<"-point projective plane of order "<<p<<", which is representable if and only if the corresponding Field has characteristic "<<p<<"."<<endl;
   }
   return m;
}


UserFunction4perl("# @category Producing a matroid from scratch\n"
                  "# Creates the projective plane matroid of rank 3 with //p**2+p+1// elements, where p is a prime."
                  "# @param Integer p"
                  "# @return Matroid",
                  &projective_plane, "projective_plane");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
 
