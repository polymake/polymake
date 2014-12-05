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
#include "polymake/list"

namespace polymake { namespace matroid {
namespace {

Array< Set<int> > connected_componets_from_circuits(const Set< Set<int> > circuits, const int n)//cast Array to Set, sorting the sets
{
   if(circuits.empty()){
      return all_subsets_of_1(range(0,n-1));
   }
   std::list< Set<int> > list;
   Set<int> element;
   Set<int> component;
   Set<int> coloops=range(0,n-1);
   Entire< Set< Set<int> > >::const_iterator i=entire(circuits);
   element+=(*i).front();
   for (; !i.at_end(); ++i) { //using [Oxley:matroid theory (2nd ed.) Cor. 4.1.3] 
      if(incl(element,*i)<1){
         component+=*i;
      }else{
         coloops-=component;
         if(incl(*i,coloops)<1){
            list.push_back(component);
            element.clear();
            component=*i;
            element+=(*i).front();
         }      
      }
   }
   list.push_back(component);
   coloops-=component;

   Array< Set<int> > result(list);
   result.append(coloops.size(),entire(coloops));
   return result;
}

}//end anonymous namespace

Function4perl(&connected_componets_from_circuits, "connected_componets_from_circuits");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
