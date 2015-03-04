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

#include "polymake/topaz/connected_sum.h"
#include "polymake/topaz/complex_tools.h"
#include <sstream>

namespace polymake { namespace topaz {
        
typedef hash_map<int,int> map;

template <typename Complex_1, typename Complex_2>
std::list< Set<int> > connected_sum(const Complex_1& C1,
                                    const Complex_2& C2,
                                    const int f1, const int f2,
                                    Array<std::string>& L1,
                                    const Array<std::string>& L2,
                                    map& P)
{
   typedef typename Complex_2::value_type Facet2;
   std::list< Set<int> > CS;
      
   // add facets of C1, omitting f1, and compute the vertex set of C1
   Set<int> V1, V2, facet1, facet2;
   int i=0;
   for (typename Entire<Complex_1>::const_iterator c_it=entire(C1); !c_it.at_end(); ++c_it, ++i) {
      if (i==f1)
         facet1=*c_it;
      else
         CS.push_back(*c_it);
      V1 += *c_it;
   }
      
   if (facet1.empty())
      throw std::runtime_error("connected_sum - f1 is not a facet index");
      
   // find facet f2 and compute the vertex set of C2
   i=0;
   for (typename Entire<Complex_2>::const_iterator c_it=entire(C2); !c_it.at_end(); ++c_it, ++i) {
      if (i==f2)
         facet2=*c_it;
      V2 += *c_it;
   }

   if (facet2.empty())
      throw std::runtime_error("connected_sum - f2 is not a facet index");
      
   if (facet1.size()!=facet2.size())
      throw std::runtime_error("connected_sum - facets dimension mismatch");
      
   // compute new vertex indices for V2, identyfing facet1 and facet2
   int index_diff= V1.back()-V2.front()+1;
   map vertex_map(V2.size());
   typename Set<int>::iterator f1_it=facet1.begin();
   for (typename Entire< Set<int> >::iterator it=entire(V2); !it.at_end(); ++it) {
      const int v=*it;
      if (facet2.contains(v)) {
         vertex_map[*it]= P.empty() ? *f1_it : P[*f1_it];
         ++f1_it;
         --index_diff;
      } else
         vertex_map[*it]=*it+index_diff;
   }
      
   // add facets of C2, omitting f2, and adjust the vertex indices
   i=0;
   for (typename Entire<Complex_2>::const_iterator c_it=entire(C2);
        !c_it.at_end(); ++c_it, ++i)
      if (i!=f2) {
         Set<int> f;
         for (typename Entire<Facet2>::const_iterator f_it=entire(*c_it);
              !f_it.at_end(); ++f_it)
            f+=vertex_map[*f_it];
         CS.push_back(f);
      }
      
   // adjust labels
   if (L1.size() != 0) {         // if L1.size()==0 => no L1 and L2 specified.
      int count = L1.size();
      L1.resize(count + L2.size() - facet1.size());
         
      for (int v=0; v<count; ++v)
         if (!facet1.contains(v)) {
            std::ostringstream label;
            label << L1[v] << "_1";
            L1[v] = label.str();
         }
         
      for (int v=0; v<L2.size(); ++v)
         if (!facet2.contains(v)) {
            std::ostringstream label;
            label << L2[v] << "_2";
            std::string l = label.str();
            L1[count] = label.str();;
            ++count;
         }
   }
      
   const Set<int> V = accumulate(CS, operations::add());
   if (adj_numbering(CS,V) && L1.size()!=0) {

      // adjust labels
      Array<std::string> L_tmp(V.size());
      Array<std::string>::iterator l = L_tmp.begin();
      for (Entire< Set<int> >::const_iterator v=entire(V);
           !v.at_end(); ++v, ++l)
         *l = L1[*v];
         
      L1 = L_tmp;
   }
      
   return CS;
}
   
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
