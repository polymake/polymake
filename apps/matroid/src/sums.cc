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
#include "polymake/Array.h"
#include "polymake/PowerSet.h"
#include "polymake/Matrix.h"
#include "polymake/Rational.h"
#include "polymake/linalg.h"
#include "polymake/list"
#include "polymake/Integer.h"

namespace polymake { namespace matroid {
namespace operations {

using namespace polymake::operations;

template <typename SetRef>
struct contains {
   typename pm::deref<SetRef>::type::element_type elem;
public:
   typedef typename pm::deref<SetRef>::type set_type;
   typedef set_type argument_type;
   typedef bool result_type;

   contains(typename set_type::element_type e) : elem(e) {}

   result_type operator() (typename pm::function_argument<SetRef>::const_type set) const
   {
      return set.contains(elem);
   }
};

template <typename Scalar>
struct dropshift {
   typedef typename pm::deref<Scalar>::type scalar_type;
   scalar_type elem;
public:
   typedef scalar_type argument_type;
   typedef scalar_type result_type;

   dropshift(scalar_type e) : elem(e) {}

   result_type operator() (argument_type value) const
   {
      return value < elem ? value : value-1;
   }
};

}

namespace {

template <typename Container>
typename pm::enable_if<
   pm::SelectedSubset<const Container,operations::contains<Set<int> > > , 
   pm::isomorphic_to_container_of<Container, Set<int> >::value>::type
select_k(const Container& sets, const int k)
{
   return pm::SelectedSubset<const Container,operations::contains<Set<int> > >(sets,operations::contains<Set<int> >(k));
}

template <typename Container>
typename pm::enable_if<
   pm::SelectedSubset<const Container,operations::composed11<operations::contains<Set<int> >,std::logical_not<bool> > >,
   pm::isomorphic_to_container_of<Container, Set<int> >::value>::type
select_not_k(const Container& sets, const int k)
{
   return pm::SelectedSubset<const Container,operations::composed11<operations::contains<Set<int> >,std::logical_not<bool> > >(sets,operations::composed11<operations::contains<Set<int> >,std::logical_not<bool> >(operations::contains<Set<int> >(k),std::logical_not<bool>()));
}

template <typename Container>
typename pm::enable_if<
   pm::TransformedContainer<const pm::TransformedContainer<const Container&, pm::operations::construct_unary2_with_arg<pm::SelectedSubset, pm::operations::fix2<int, operations::ne> > >&, pm::operations::construct_unary2_with_arg<pm::TransformedContainer, operations::dropshift<int> > >,
   pm::isomorphic_to_container_of<Container, Set<int> >::value>::type
drop_shift(const Container& sets, const int k)
{
   return attach_operation(
            attach_operation(sets, pm::operations::construct_unary2_with_arg< pm::SelectedSubset, operations::fix2<int,operations::ne> >(operations::fix2<int,operations::ne>(k))),
         pm::operations::construct_unary2_with_arg<pm::TransformedContainer, operations::dropshift<int> >(operations::dropshift<int>(k)));
}

Array<Set <int> > shift_elements(const Array< Set<int> >& sets, const int n)
{
      return attach_operation(
                              sets, 
                              pm::operations::construct_unary2_with_arg<pm::TransformedContainer, 
                              operations::fix2<int,operations::add> >(operations::fix2<int,operations::add>(n))
                              );
}

perl::Object direct_sum(perl::Object m1, perl::Object m2)
{
   const int n1=m1.give("N_ELEMENTS");
   const int n2=m2.give("N_ELEMENTS");

   perl::Object m_new("Matroid");
   m_new.set_description()<<"The direct-sum of "<<m1.name()<<" and "<<m2.name()<<"."<<endl;
   m_new.take("N_ELEMENTS")<<(n1+n2);

   const int rank1=m1.give("RANK");
   const int rank2=m2.give("RANK");
   m_new.take("RANK") << rank1+rank2;

   Matrix<Rational> points1,points2;
   if (m1.lookup("POINTS")>>points1 && m2.lookup("POINTS")>>points2){
      m_new.take("POINTS") << ( points1|Matrix<Rational>(n1,points2.cols()) ) / ( (Matrix<Rational>(n2,points1.cols()))|points2 );
   }
   if (m1.lookup("BINARY_POINTS")>>points1 && m2.lookup("BINARY_POINTS")>>points2){
      if(m1.give("BINARY") && m2.give("BINARY")){
         m_new.take("BINARY_POINTS") << ( points1|Matrix<Rational>(n1,rank2) ) / ( (Matrix<Rational>(n2,rank1))|points2 );
         m_new.take("BINARY") << 1;
      }else{
         m_new.take("BINARY") << 0;
      }
   }

   bool def=0;
   Array<Set<int> > sets1, sets2; //caution: these  variables  will be overwritten
   if ((m1.lookup("BASES")>>sets1 && m2.lookup("BASES")>>sets2) ) {
      m_new.take("BASES") << product(sets1,shift_elements(sets2,n1),operations::add());
      def=1;
   }
   if (m1.lookup("CIRCUITS")>>sets1 && m2.lookup("CIRCUITS")>>sets2) {
      sets1.append( sets2.size() , entire(shift_elements(sets2,n1)) );
      m_new.take("CIRCUITS") << sets1;
      def=1;
   }
   if (m1.lookup("COCIRCUITS")>>sets1 && m2.lookup("COCIRCUITS")>>sets2) {
      sets1.append( sets2.size() , entire(shift_elements(sets2,n1)) );
      m_new.take("COCIRCUITS") << sets1;
      def=1;
   }
   if (m1.lookup("MATROID_HYPERPLANES")>>sets1 && m2.lookup("MATROID_HYPERPLANES")>>sets2) {
      for (Entire< Array< Set<int> > >::iterator i=entire(sets1); !i.at_end(); ++i) {
         *i+=range(n1,n1+n2-1);
      }
      sets2=shift_elements(sets2,n1);
      for (Entire< Array< Set<int> > >::iterator i=entire(sets2); !i.at_end(); ++i) {
         *i+=range(0,n1-1);
      }
      sets2.append( sets1.size() , entire(sets1) );   
      m_new.take("MATROID_HYPERPLANES") <<sets2 ;
   }
   if (m1.lookup("CONNECTED_COMPONENTS")>>sets1 && m2.lookup("CONNECTED_COMPONENTS")>>sets2) {
      sets1.append( sets2.size() , entire(shift_elements(sets2,n1)) );
      m_new.take("CONNECTED_COMPONENTS")<<sets1;
   }

   if(def==0){
      sets1=m1.give("BASES");
      sets2=m2.give("BASES");
      m_new.take("BASES") << product(sets1,shift_elements(sets2,n1),operations::add());
   }

   m_new.take("CONNECTED") << 0;

   return m_new;
}


perl::Object series_extension(perl::Object m1, const int e1, perl::Object m2, const int e2)
{
   const int n1=m1.give("N_ELEMENTS");
   const int n2=m2.give("N_ELEMENTS");
   if (e1<0||e1>=n1 || e2<0||e2>=n2) throw std::runtime_error("series-extansion: element out of bounds");

   //coloop check:
   const Set<int>& temp1 = m1.give("COLOOPS");
   const Set<int>& temp2 = m2.give("COLOOPS");
   if(temp1.size()>0 && temp2.size()>0) throw std::runtime_error("series-extansion: both basepoints are coloops");

   perl::Object m_new("Matroid");
   m_new.set_description()<<"The series extansion of "<<m1.name()<<" and "<<m2.name()<<", with basepoints "<< e1 <<" and "<< e2 <<"."<<endl;
   m_new.take("N_ELEMENTS")<<(n1+n2-1);

   Array< Set<int> > sets1,sets2,sets,result;   
   sets1=m1.give("BASES");
   sets2=m2.give("BASES");
   result=product(select_k(sets1,e1),shift_elements(drop_shift(select_not_k(sets2,e2),e2),n1),operations::add());
   sets=product(select_not_k(sets1,e1)
                ,attach_operation(shift_elements(drop_shift(select_k(sets2,e2),e2),n1),operations::fix2<int,operations::add>(operations::fix2<int,operations::add>(e1)))
                ,operations::add());
   result.append(sets.size(),entire(sets));
   sets=product(select_not_k(sets1,e1),shift_elements(drop_shift(select_not_k(sets2,e2),e2),n1),operations::add());
   result.append(sets.size(),entire(sets));
   m_new.take("BASES") <<result;

   return m_new;
}


perl::Object single_element_series_extension(perl::Object m, const int e) //with Uniform(1,2)
{
   const int n=m.give("N_ELEMENTS");
   if (e<0||e>=n) throw std::runtime_error("series-extension: element out of bounds");

   perl::Object m_new("Matroid");
   m_new.set_description()<<"The series extension of "<<m.name()<<" and U(1,2), with basepoints "<< e <<"."<<endl;
   m_new.take("N_ELEMENTS")<<(n+1);

   Array<Set<int> > bases=m.give("BASES");
   std::list< Set<int> > new_bases;
   for (Entire< Array< Set<int> > >::const_iterator i=entire(bases); !i.at_end(); ++i) {
      new_bases.push_back(*i+n);
      if(!(i->contains(e))){
         new_bases.push_back(*i+e);
      }
   }
   m_new.take("BASES") <<new_bases;
   return m_new;
}

perl::Object parallel_extension(perl::Object m1, const int e1, perl::Object m2, const int e2)
{
   const int n1=m1.give("N_ELEMENTS");
   const int n2=m2.give("N_ELEMENTS");
   if (e1<0||e1>=n1 || e2<0||e2>=n2) throw std::runtime_error("series-extansion: element out of bounds");

   //loop check:
   const Set<int>& temp1 = m1.give("LOOPS");
   const Set<int>& temp2 = m2.give("LOOPS");
   if(temp1.size()>0 && temp2.size()>0) throw std::runtime_error("series-extansion: both basepoints are loops");

   perl::Object m_new("Matroid");
   m_new.set_description()<<"The parallel extansion of "<<m1.name()<<" and "<<m2.name()<<", with basepoints "<< e1 <<" and "<< e2 <<"."<<endl;
   m_new.take("N_ELEMENTS")<<(n1+n2-1);

   Array< Set<int> > sets1,sets2,result,sets;
   
   sets1=m1.give("BASES");
   sets2=m2.give("BASES");

   result=product(select_k(sets1,e1),shift_elements(drop_shift(select_k(sets2,e2),e2),n1),operations::add());
   sets=product(select_not_k(sets1,e1),shift_elements(drop_shift(select_k(sets2,e2),e2),n1),operations::add());
   result.append(sets.size(),entire(sets));
   //overwritting sets1 with a set where the elements are those elements that contain e1 in the former set and e1 is removed. 
   sets1=attach_operation(select_k(sets1,e1), pm::operations::construct_unary2_with_arg< pm::SelectedSubset, operations::fix2<int,operations::ne> >(operations::fix2<int,operations::ne>(e1)));
   sets=product(sets1,shift_elements(drop_shift(select_not_k(sets2,e2),e2),n1),operations::add());
   result.append(sets.size(),entire(sets));

   m_new.take("BASES") <<result;

   return m_new;
}


perl::Object single_element_parallel_extension(perl::Object m, const int e) //with Uniform(1,2)
{
   const int n=m.give("N_ELEMENTS");
   if (e<0||e>=n) throw std::runtime_error("parallel-extension: element out of bounds");

   perl::Object m_new("Matroid");
   m_new.set_description()<<"The parallel extension of "<<m.name()<<" and U(1,2), with basepoint "<< e <<"."<<endl;
   m_new.take("N_ELEMENTS")<<(n+1);

   Array<Set<int> > bases=m.give("BASES");
   std::list< Set<int> > new_bases;
   int count=0;   
   for (Entire< Array< Set<int> > >::const_iterator i=entire(bases); !i.at_end(); ++i) {
      if(i->contains(e)){
         new_bases.push_back(*i-e+n);
         ++count;
      }
   }
   bases.append(count,entire(new_bases));
   m_new.take("BASES") <<bases;
   return m_new;
}

perl::Object two_sum(perl::Object m1, const int e1, perl::Object m2, const int e2)
{
   const int n1=m1.give("N_ELEMENTS");
   const int n2=m2.give("N_ELEMENTS");
   if (e1<0||e1>=n1 || e2<0||e2>=n2) throw std::runtime_error("2-sum: element out of bounds");

   //loop check:
   const Set<int>& temp1 = m1.give("COLOOPS");
   const Set<int>& temp2 = m2.give("COLOOPS");
   if(temp1.size()>0 && temp2.size()>0) throw std::runtime_error("2-sum: both basepoints are coloops");

   perl::Object m_new("Matroid");
   m_new.set_description()<<"The 2-sum of "<<m1.name()<<" and "<<m2.name()<<", with basepoints "<< e1 <<" and "<< e2 <<"."<<endl;
   m_new.take("N_ELEMENTS")<<(n1+n2-2);

   const int rank1=m1.give("RANK");
   const int rank2=m2.give("RANK");
   m_new.take("RANK") << rank1+rank2-1;

   Array< Set<int> > sets1,sets2,result,sets;
   sets1=m1.give("BASES");
   sets2=m2.give("BASES");
   
   result=product(Array< Set<int> >(drop_shift(select_k(sets1,e1),e1)),shift_elements(drop_shift(select_not_k(sets2,e2),e2),n1-1),operations::add());
   sets=product(Array< Set<int> >(drop_shift(select_not_k(sets1,e1),e1)),shift_elements(drop_shift(select_k(sets2,e2),e2),n1-1),operations::add());
   result.append(sets.size(),entire(sets));
   m_new.take("BASES") <<result;

   return m_new;
}


}//end anonymous namespace

UserFunction4perl("# @category Producing a matroid from matroids"
                  "# The direct sum of matroids m1 and m2"
                  "# @param Matroid m_1"
                  "# @param Matroid m_2"
                  "# @return Matroid",
                  &direct_sum,"direct_sum");
UserFunction4perl("# @category Producing a matroid from matroids"
                  "# The series extension of matroids m1 and m2 with basepoints e1 and e2"
                  "# @param Matroid m_1"
                  "# @param int e_1"
                  "# @param Matroid m_2"
                  "# @param int e_2"
                  "# @return Matroid",
                  &series_extension,"series_extension(Matroid $ Matroid $)");
UserFunction4perl("# @category Producing a matroid from matroids"
                  "# The series extension of a matroid m and uniform(2,4) with basepoint e"
                  "# @param Matroid m"
                  "# @param int e"
                  "# @return Matroid",
                  &single_element_series_extension,"series_extension(Matroid $)");
UserFunction4perl("# @category Producing a matroid from matroids"
                  "# The parallel extension of matroids m1 and m2 with basepoints e1 and e2"
                  "# @param Matroid m_1"
                  "# @param int e_1"
                  "# @param Matroid m_2"
                  "# @param int e_2"
                  "# @return Matroid",
                  &parallel_extension,"parallel_extension(Matroid $ Matroid $)");
UserFunction4perl("# @category Producing a matroid from matroids"
                  "# The parallel extension of a matroid m and uniform(2,4) with basepoint e"
                  "# @param Matroid m"
                  "# @param int e"
                  "# @return Matroid",
                  &single_element_parallel_extension,"parallel_extension(Matroid $)");
UserFunction4perl("# @category Producing a matroid from matroids"
                  "# The 2-sum of matroids m1 and m2  with basepoints e1 and e2"
                  "# @param Matroid m_1"
                  "# @param int e_1"
                  "# @param Matroid m_2"
                  "# @param int e_2"
                  "# @return Matroid",
                  &two_sum,"two_sum");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
