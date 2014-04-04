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
#include "polymake/Matrix.h"
#include "polymake/Rational.h"
#include "polymake/PowerSet.h"
#include "polymake/Array.h"
#include "polymake/list"

namespace polymake { namespace matroid {

/*
 * calculates the rank of a set in a matroid given by his bases 
 */
int rank_of_set(const Set<int> set, const Set<Set<int> > bases){
   int rank=0;
   for (Entire<Set<Set<int> > >::const_iterator it=entire(bases);!it.at_end();++it){
      int r=((*it)*set).size();
      if(r>rank)
         rank=r;
   }
   return rank;
}

perl::ListReturn matroid_plueckervector(perl::Object matroid){
   const Set< Set<int> > bases=matroid.give("BASES");
   const int r = matroid.give("RANK");
   const int n = matroid.give("N_ELEMENTS");
   Vector<int> char_vec(Integer::binom(n,r).to_int());
   Vector<int> rank_vec(Integer::binom(n,r).to_int());

   int l=0;
   for (Entire< Subsets_of_k<const sequence&> >::const_iterator i=entire(all_subsets_of_k(sequence(0,n),r)); !i.at_end(); ++i,++l){
      if(bases.contains(*i)){
         char_vec[l]=1;
         rank_vec[l]=r;
      }else{
         rank_vec[l]=rank_of_set(*i,bases);
      }
   }

   perl::ListReturn list_ret;
   list_ret << char_vec
	    << rank_vec;
   return list_ret;
}

perl::Object matroid_from_characteristic_vector(const Vector<Integer> vec,const int r,const int n){ 
   if(vec.dim()!=Integer::binom(n,r).to_int()){
      throw std::runtime_error("matroid_from_characteristic_vector: dimension of the vector does not fit with the given rank and the number of elments");
   }
   perl::Object m("Matroid");
   std::list< Set<int> > bases;
   int n_bases=0;
   int j=0;
  
   //test for each subset of size r
   for (Entire< Subsets_of_k<const sequence&> >::const_iterator i=entire(all_subsets_of_k(sequence(0,n),r)); !i.at_end(); ++i,++j) {    
      if (vec[j]==1) {
	 bases.push_back(*i);
	 ++n_bases;
      }
   }
   
   m.take("BASES") << bases;
   m.take("N_BASES") << n_bases;
   m.take("RANK") << r;
   m.take("N_ELEMENTS") << n;
   return m;
}

UserFunction4perl("# @category Producing from scratch\n"
                  "# Creates the matroid with a given characteristic-plueckervector of rank r and a ground set of n elements."
                  "# @param Vector<Integer> v"
                  "# @param int r"
                  "# @param int n"
                  "# @return Matroid",
                  &matroid_from_characteristic_vector, "matroid_from_characteristic_vector");

UserFunction4perl("# @category Producing plueckervectors\n"
                  "# Creates the characteristic- and the rank-plueckervector of a matroid."
                  "# @param Matroid m"
                  "# @return ListReturn (Vector<Integer>, Vector<Integer>)",
                  &matroid_plueckervector, "matroid_plueckervector");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
