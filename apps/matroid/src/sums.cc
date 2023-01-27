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
#include "polymake/Array.h"
#include "polymake/PowerSet.h"
#include "polymake/Matrix.h"
#include "polymake/Rational.h"
#include "polymake/linalg.h"
#include "polymake/list"
#include "polymake/Integer.h"
#include "polymake/matroid/util.h"

namespace polymake { namespace matroid {

BigObject direct_sum(BigObject m1, BigObject m2)
{
   const Int n1 = m1.give("N_ELEMENTS");
   const Int n2 = m2.give("N_ELEMENTS");

   BigObject m_new("Matroid");
   m_new.set_description()<<"The direct-sum of "<<m1.name()<<" and "<<m2.name()<<"."<<endl;
   m_new.take("N_ELEMENTS")<<(n1+n2);

   const Int rank1 = m1.give("RANK");
   const Int rank2 = m2.give("RANK");
   m_new.take("RANK") << rank1+rank2;

   Matrix<Rational> points1,points2;
   if (m1.lookup("VECTORS")>>points1 && m2.lookup("VECTORS")>>points2){
      m_new.take("VECTORS") << ( points1|Matrix<Rational>(n1,points2.cols()) ) / ( (Matrix<Rational>(n2,points1.cols()))|points2 );
   }
   if (m1.lookup("BINARY_VECTORS")>>points1 && m2.lookup("BINARY_VECTORS")>>points2){
      if(m1.give("BINARY") && m2.give("BINARY")){
         m_new.take("BINARY_VECTORS") << ( points1|Matrix<Rational>(n1,rank2) ) / ( (Matrix<Rational>(n2,rank1))|points2 );
         m_new.take("BINARY") << true;
      }else{
         m_new.take("BINARY") << false;
      }
   }

   bool def = false;
   Array<Set<Int>> sets1, sets2; //caution: these variables will be overwritten
   if (m1.lookup("BASES")>>sets1 && m2.lookup("BASES")>>sets2) {
      m_new.take("BASES") << product(sets1, shift_elements(sets2, n1),operations::add());
      def = true;
   }
   if (m1.lookup("CIRCUITS")>>sets1 && m2.lookup("CIRCUITS")>>sets2) {
      sets1.append(shift_elements(sets2, n1));
      m_new.take("CIRCUITS") << sets1;
      def = true;
   }
   if (m1.lookup("DUAL.CIRCUITS")>>sets1 && m2.lookup("DUAL.CIRCUITS")>>sets2) {
      sets1.append(shift_elements(sets2, n1));
      m_new.take("DUAL.CIRCUITS") << sets1;
      def = true;
   }
   if (m1.lookup("MATROID_HYPERPLANES")>>sets1 && m2.lookup("MATROID_HYPERPLANES")>>sets2) {
      for (auto& s1 : sets1) {
         s1 += range(n1, n1+n2-1);
      }
      sets2 = shift_elements(sets2, n1);
      for (auto& s2 : sets2) {
         s2 += range(0, n1-1);
      }
      sets2.append(sets1);   
      m_new.take("MATROID_HYPERPLANES") << sets2;
   }
   if (m1.lookup("CONNECTED_COMPONENTS")>>sets1 && m2.lookup("CONNECTED_COMPONENTS")>>sets2) {
      sets1.append(shift_elements(sets2, n1));
      m_new.take("CONNECTED_COMPONENTS") << sets1;
   }

   if (!def) {
      m1.give("BASES") >> sets1;
      m2.give("BASES") >> sets2;
      m_new.take("BASES") << product(sets1,shift_elements(sets2, n1),operations::add());
   }

   m_new.take("CONNECTED") << false;

   return m_new;
}


BigObject series_extension(BigObject m1, const Int e1, BigObject m2, const Int e2)
{
   const Int n1 = m1.give("N_ELEMENTS");
   const Int n2 = m2.give("N_ELEMENTS");
   if (e1 < 0 || e1 >= n1 || e2 < 0 || e2 >= n2) throw std::runtime_error("series-extansion: element out of bounds");

   //coloop check:
   const Set<Int>& temp1 = m1.give("DUAL.LOOPS");
   const Set<Int>& temp2 = m2.give("DUAL.LOOPS");
   if (temp1.size()>0 && temp2.size()>0) throw std::runtime_error("series-extansion: both basepoints are coloops");


   const Array<Set<Int>> sets1 = m1.give("BASES");
   const Array<Set<Int>> sets2 = m2.give("BASES");
   Array<Set<Int>> bases( product(select_k(sets1, e1),
                                  shift_elements(drop_shift(select_not_k(sets2, e2), e2), n1),
                                  operations::add()),
                          product(select_not_k(sets1, e1),
                                  attach_operation(shift_elements(drop_shift(select_k(sets2, e2), e2), n1), operations::fix2<Int, operations::add>(operations::fix2<Int, operations::add>(e1))),
                                  operations::add()),
                          product(select_not_k(sets1, e1),
                                  shift_elements(drop_shift(select_not_k(sets2,e2), e2), n1),
                                  operations::add()));

   BigObject m_new("Matroid",
                   "N_ELEMENTS", n1+n2-1,
                   "BASES", bases);
   m_new.set_description()<<"The series extansion of "<<m1.name()<<" and "<<m2.name()<<", with basepoints "<< e1 <<" and "<< e2 <<"."<<endl;

   return m_new;
}


BigObject single_element_series_extension(BigObject m, const Int e) //with Uniform(1,2)
{
   const Int n = m.give("N_ELEMENTS");
   if (e < 0 || e >= n) throw std::runtime_error("series-extension: element out of bounds");

   Array<Set<Int>> bases = m.give("BASES");
   std::list<Set<Int>> new_bases;
   for (auto i = entire(bases); !i.at_end(); ++i) {
      new_bases.push_back(*i+n);
      if (!(i->contains(e))) {
         new_bases.push_back(*i+e);
      }
   }

   BigObject m_new("Matroid",
                   "N_ELEMENTS", n+1,
                   "BASES", new_bases);
   m_new.set_description()<<"The series extension of "<<m.name()<<" and U(1,2), with basepoints "<< e <<"."<<endl;
   return m_new;
}

BigObject parallel_extension(BigObject m1, const Int e1, BigObject m2, const Int e2)
{
   const Int n1 = m1.give("N_ELEMENTS");
   const Int n2 = m2.give("N_ELEMENTS");
   if (e1 < 0 || e1 >= n1 || e2 < 0 || e2 >= n2) throw std::runtime_error("series-extansion: element out of bounds");

   //loop check:
   const Set<Int>& temp1 = m1.give("LOOPS");
   const Set<Int>& temp2 = m2.give("LOOPS");
   if (temp1.size()>0 && temp2.size()>0) throw std::runtime_error("series-extansion: both basepoints are loops");

   const Array<Set<Int>> sets1 = m1.give("BASES");
   const Array<Set<Int>> sets2 = m2.give("BASES");
   // a set where the elements are those elements of sets1 that contain e1 in the former set and e1 is removed. 
   const Array<Set<Int>> sets3(attach_operation(select_k(sets1,e1), pm::operations::construct_unary2_with_arg<pm::SelectedSubset, operations::fix2<Int, operations::ne> >(operations::fix2<Int, operations::ne>(e1))));

   Array<Set<Int>> bases( product(select_k(sets1, e1),
                                  shift_elements(drop_shift(select_k(sets2, e2), e2), n1),
                                  operations::add()),
                          product(select_not_k(sets1, e1),
                                  shift_elements(drop_shift(select_k(sets2, e2), e2), n1),
                                  operations::add()),
                          product(sets3,
                                  shift_elements(drop_shift(select_not_k(sets2, e2), e2), n1),
                                  operations::add()));

   BigObject m_new("Matroid",
                   "N_ELEMENTS", n1+n2-1,
                   "BASES", bases);
   m_new.set_description()<<"The parallel extansion of "<<m1.name()<<" and "<<m2.name()<<", with basepoints "<< e1 <<" and "<< e2 <<"."<<endl;
   return m_new;
}


BigObject single_element_parallel_extension(BigObject m, const Int e) //with Uniform(1,2)
{
   const Int n = m.give("N_ELEMENTS");
   if (e < 0 || e >= n) throw std::runtime_error("parallel-extension: element out of bounds");

   Array<Set<Int>> bases = m.give("BASES");
   std::list<Set<Int>> new_bases;
   Int count = 0;
   for (auto i = entire(bases); !i.at_end(); ++i) {
      if (i->contains(e)) {
         new_bases.push_back(*i-e+n);
         ++count;
      }
   }
   bases.append(count, new_bases.begin());

   BigObject m_new("Matroid",
                   "N_ELEMENTS", n+1,
                   "BASES", bases);
   m_new.set_description()<<"The parallel extension of "<<m.name()<<" and U(1,2), with basepoint "<< e <<"."<<endl;
   return m_new;
}

BigObject two_sum(BigObject m1, const Int e1, BigObject m2, const Int e2)
{
   const Int n1 = m1.give("N_ELEMENTS");
   const Int n2 = m2.give("N_ELEMENTS");
   if (e1 < 0 || e1 >= n1 || e2 < 0 || e2 >= n2) throw std::runtime_error("2-sum: element out of bounds");

   //loop check:
   const Set<Int>& temp1 = m1.give("DUAL.LOOPS");
   const Set<Int>& temp2 = m2.give("DUAL.LOOPS");
   if (temp1.size()>0 && temp2.size()>0) throw std::runtime_error("2-sum: both basepoints are coloops");

   const Int rank1 = m1.give("RANK");
   const Int rank2 = m2.give("RANK");

   const Array<Set<Int>> sets1 = m1.give("BASES");
   const Array<Set<Int>> sets2 = m2.give("BASES");

   Array<Set<Int>> bases( product(Array<Set<Int>>(drop_shift(select_k(sets1, e1), e1)),
                                  shift_elements(drop_shift(select_not_k(sets2, e2), e2), n1-1),
                                  operations::add()),
                          product(Array<Set<Int>>(drop_shift(select_not_k(sets1, e1), e1)),
                                  shift_elements(drop_shift(select_k(sets2, e2), e2), n1-1),
                                  operations::add()));

   BigObject m_new("Matroid",
                   "N_ELEMENTS", n1+n2-2,
                   "RANK", rank1+rank2-1,
                   "BASES", bases);
   m_new.set_description()<<"The 2-sum of "<<m1.name()<<" and "<<m2.name()<<", with basepoints "<< e1 <<" and "<< e2 <<"."<<endl;
   return m_new;
}

UserFunction4perl("# @category Producing a matroid from matroids"
                  "# The direct sum of matroids m1 and m2"
                  "# @param Matroid m_1"
                  "# @param Matroid m_2"
                  "# @return Matroid",
                  &direct_sum,"direct_sum");
UserFunction4perl("# @category Producing a matroid from matroids"
                  "# The series extension of matroids m1 and m2 with basepoints e1 and e2"
                  "# @param Matroid m_1"
                  "# @param Int e_1"
                  "# @param Matroid m_2"
                  "# @param Int e_2"
                  "# @return Matroid",
                  &series_extension,"series_extension(Matroid $ Matroid $)");
UserFunction4perl("# @category Producing a matroid from matroids"
                  "# The series extension of a matroid m and uniform(1,2) with basepoint e"
                  "# @param Matroid m"
                  "# @param Int e"
                  "# @return Matroid",
                  &single_element_series_extension,"series_extension(Matroid $)");
UserFunction4perl("# @category Producing a matroid from matroids"
                  "# The parallel extension of matroids m1 and m2 with basepoints e1 and e2"
                  "# @param Matroid m_1"
                  "# @param Int e_1"
                  "# @param Matroid m_2"
                  "# @param Int e_2"
                  "# @return Matroid",
                  &parallel_extension,"parallel_extension(Matroid $ Matroid $)");
UserFunction4perl("# @category Producing a matroid from matroids"
                  "# The parallel extension of a matroid m and uniform(1,2) with basepoint e"
                  "# @param Matroid m"
                  "# @param Int e"
                  "# @return Matroid",
                  &single_element_parallel_extension,"parallel_extension(Matroid $)");
UserFunction4perl("# @category Producing a matroid from matroids"
                  "# The 2-sum of matroids m1 and m2  with basepoints e1 and e2"
                  "# @param Matroid m_1"
                  "# @param Int e_1"
                  "# @param Matroid m_2"
                  "# @param Int e_2"
                  "# @return Matroid",
                  &two_sum,"two_sum");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
