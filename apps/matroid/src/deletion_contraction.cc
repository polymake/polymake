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
namespace {

enum { element_not_found, element_found, break_when_element_found };

inline
Set<int> reduce_set(const Set<int>& set, const int element, int& found_flag)
{
  Set<int> new_set;
  for (Entire< Set<int> >::const_iterator j=entire(set); !j.at_end(); ++j) {
    if (*j == element) {
      if (found_flag==break_when_element_found) {
        found_flag=element_found;
        break;
      }
      found_flag=element_found;
    } else {
      new_set.push_back(*j<element ? *j : (*j)-1);
    }
  }

  return new_set;
}

template <typename _prefer_containing>
Array< Set<int> > collect_bases(const Array< Set<int> >& bases, const int element)
{
  //_prefer_contaning == 'false' for deletion and 'true' for contraction
  std::list< Set<int> > not_containing_bases, containing_bases;
  for (Entire< Array< Set<int> > >::const_iterator i=entire(bases); !i.at_end(); ++i) {
    int found_flag= _prefer_containing::value || not_containing_bases.empty() ? element_not_found : break_when_element_found;
    Set<int> new_basis=reduce_set(*i, element, found_flag);
    if (found_flag==element_found) {
      if ((_prefer_containing::value || not_containing_bases.empty()) )
        containing_bases.push_back(new_basis);
    } else {
       if (!_prefer_containing::value || containing_bases.empty()){
        not_containing_bases.push_back(new_basis);
       }
    }
  }  
  if (containing_bases.empty())
    return not_containing_bases;
  else
    if (_prefer_containing::value || not_containing_bases.empty())
    return containing_bases;
  else
    return not_containing_bases;
}

template <typename _contraction>
Array< Set<int> > collect_non_bases(const Array< Set<int> >& nonbases, const int element, int& count){
   //_contraction == 'False' for deleation and 'True' for contraction
   //count = maximal number of elements in containing_nonbases or not_containing_nonbases

  std::list< Set<int> > not_containing_nonbases, containing_nonbases;
  for (Entire< Array< Set<int> > >::const_iterator i=entire(nonbases); !i.at_end(); ++i) {
    int found_flag=element_not_found;
    Set<int> new_nonbasis=reduce_set(*i, element, found_flag);
    if (found_flag==element_found) {
       containing_nonbases.push_back(new_nonbasis);
       if(_contraction::value){
          --count;
          if(count==0){
             if(not_containing_nonbases.empty())
                count=-1;
             return not_containing_nonbases;
          }
       }
    } else {
        not_containing_nonbases.push_back(new_nonbasis);
        if(!_contraction::value){
           --count;
           if(count==0){
              if(containing_nonbases.empty())
                 count=-1;
              return containing_nonbases;
           }
        }
    }
  }

  if (_contraction::value){
     if(containing_nonbases.empty())
        count=-2;
     return containing_nonbases;
  }

  if(not_containing_nonbases.empty())
     count=-2;
  return not_containing_nonbases;
}

Array< Set<int> >
collect_not_containing_circuits(const Array< Set<int> >& circuits, int element)
{
  std::list< Set<int> > new_circuits;
  for (Entire< Array< Set<int> > >::const_iterator i=entire(circuits); !i.at_end(); ++i) {
    int found_flag=break_when_element_found;
    Set<int> new_circ=reduce_set(*i, element, found_flag);
    if (found_flag != element_found) new_circuits.push_back(new_circ);
  }
  return new_circuits;
}

Array< Set<int> >
collect_circuits(const Array< Set<int> >& circuits, int element)
{
  std::list< Set<int> > new_circuits, candidates; //if these are minimal, they will also be new circuits

  for (Entire< Array< Set<int> > >::const_iterator i=entire(circuits); !i.at_end(); ++i) {
    int found_flag=element_not_found;
    Set<int> new_circ=reduce_set(*i, element, found_flag);
    if (found_flag == element_found) {
      if (!new_circ.empty()) new_circuits.push_back(new_circ);
    } else {
      candidates.push_back(new_circ);
    }
  }

  for (Entire< std::list< Set<int> > >::const_iterator i=entire(candidates); !i.at_end(); ++i) {
    bool is_minimal=true;
    for (Entire< std::list< Set<int> > >::const_iterator j=entire(new_circuits); !j.at_end(); ++j) {
      if (incl(*j,*i)<=0) {
        is_minimal=false;
        break;
      }
    }
    if (is_minimal) new_circuits.push_back(*i);
  }

  return new_circuits;
}

Array<std::string>
reduce_labels(const Array<std::string>& labels, int n_elements, int element)
{
  if (!labels.empty()) return select(labels, ~scalar2set(element));

  Array<std::string> new_labels(n_elements-1);
  Array<std::string>::iterator l=new_labels.begin();
  for (int i=0; i<n_elements; ++i) {
    if (i!=element) {
      std::stringstream out;
      out << i;
      *l=out.str();
      ++l;
    }
  }
  return new_labels;
}

} // end anonymous namespace

perl::Object deletion(perl::Object m, int element)
{
  const int n=m.give("N_ELEMENTS");
  if (element<0||element>=n) throw std::runtime_error("deletion: element out of bounds");

  perl::Object m_new("Matroid");
  m_new.set_description()<<"Deletion of element number "<<element<<" from matroid "<<m.name()<<"."<<endl;
  m_new.take("N_ELEMENTS") << (n-1);

  Array<std::string> labels;
  m.lookup("LABELS")>>labels;
  m_new.take("LABELS") << reduce_labels(labels, n, element);

  Array<Set<int> > bases;
  if (m.lookup("BASES")>>bases) {
     m_new.take("BASES") << collect_bases<pm::False>(bases, element);
  }

  if (m.lookup("NON_BASES")>>bases) {
     if(!bases.empty()){
        int rank = bases[0].size();
        int count=Integer::binom(n-1,rank).to_int();
        m_new.take("NON_BASES") << collect_non_bases<pm::False>(bases, element, count); //count<0: new NON_BASES is empty
        if(count==-1){
           if(rank==1) throw std::runtime_error("deletion: rank will be to low");              
           m_new.take("RANK")<< rank-1;
        }
        if(count==-2)
           m_new.take("RANK")<< rank;
     }else{ // m is the uniform matroid:
        int rank = m.give("RANK");
        m_new.take("NON_BASES") << bases;
        if(rank<n){
           m_new.take("RANK")<< rank;
        }else{
           if(rank==1) throw std::runtime_error("deletion: rank will be to low"); 
           m_new.take("RANK")<< rank-1;
        }
     }
  }

  Array<Set<int> > circuits;
  if (m.lookup("COCIRCUITS")>>circuits) {
    m_new.take("COCIRCUITS") << collect_circuits(circuits, element);
  }

  if (m.lookup("CIRCUITS")>>circuits) {
    m_new.take("CIRCUITS") << collect_not_containing_circuits(circuits, element);
  }

  Matrix<Rational> points;
  if (m.lookup("POINTS")>>points)
    m_new.take("POINTS")<<points.minor(~scalar2set(element),All);

  return m_new;
}

UserFunction4perl("# @category Producing a matroid from matroids"
                  "# The matroid obtained from a matroid //m// by __deletion__ of //element// ."
                  "# @param Matroid m"
                  "# @param Int element index of element to be deleted"
                  "# @return Matroid",
                  &deletion,"deletion(Matroid $)");

perl::Object contraction(perl::Object m, int element)
{
  const int n=m.give("N_ELEMENTS");
  if (element<0||element>=n) throw std::runtime_error("contraction: element out of bounds");

  perl::Object m_new("Matroid");
  m_new.set_description()<<"Contraction of "<<element<<"th element from "<<m.name()<<"."<<endl;
  m_new.take("N_ELEMENTS")<<(n-1);

  Array<std::string> labels;
  m.lookup("LABELS")>>labels;
  m_new.take("LABELS") << reduce_labels(labels, n, element);

  Array<Set<int> > bases;
  if (m.lookup("BASES")>>bases) {
     m_new.take("BASES") << collect_bases<pm::True>(bases, element);
  }

  if (m.lookup("NON_BASES")>>bases) {
     if(!bases.empty()){
        int rank = bases[0].size();
        int count=Integer::binom(n-1,rank-1).to_int();
        m_new.take("NON_BASES") << collect_non_bases<pm::True>(bases, element, count); //count<0: new NON_BASES is empty
        if(count==-2){            
           m_new.take("RANK")<< rank-1;
        }
        if(count==-1)
           m_new.take("RANK")<< rank;
     }else{ // m is the uniform matroid:
        int rank = m.give("RANK");
        m_new.take("NON_BASES") << bases;
        m_new.take("RANK")<< (rank ? rank-1 : 0);
     }
  }

  Array<Set<int> > circuits;
  if (m.lookup("COCIRCUITS")>>circuits) {
    m_new.take("COCIRCUITS") << collect_not_containing_circuits(circuits, element);
  }

  if (m.lookup("CIRCUITS")>>circuits) {
    m_new.take("CIRCUITS") << collect_circuits(circuits, element);
  }

  Matrix<Rational> points;
  if (m.lookup("POINTS")>>points) {
    const Matrix<Rational> ns1=null_space(T(points));
    if (ns1.rows())  {
      const Matrix<Rational> ns2=null_space(ns1.minor(All,~scalar2set(element)));
      if (ns2.rows()) m_new.take("POINTS")<< T(ns2);
      else m_new.take("POINTS")<<vector2col(zero_vector<Rational>(n-1));
    }
    else {
      m_new.take("POINTS")<< unit_matrix<Rational>(n-1);
    } 
  }

  return m_new;
}

UserFunction4perl("# @category Producing a matroid from matroids"
                  "# The matroid obtained from a matroid //m// by __contraction__ of //element// ."
                  "# @param Matroid m"
                  "# @param Int element index of element to be contracted"
                  "# @return Matroid",
                  &contraction,"contraction(Matroid $)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
