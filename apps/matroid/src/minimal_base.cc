/* Copyright (c) 1997-2019
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
#include "polymake/Vector.h"
#include "polymake/Array.h"
#include "polymake/Set.h"
#include <list>
#include <algorithm>

namespace polymake { namespace matroid {

template <typename Scalar> 
struct Comp
{
   const Vector<Scalar> weights;
   Comp(const Vector<Scalar>& v) : weights(v) {}
   bool operator () (const int &k, const int &l) { return weights[k]<weights[l]; }
};

template <typename Scalar> 
Set<int> minimal_base(perl::Object m, Vector<Scalar> weights)
{
   int n=m.give("N_ELEMENTS");
   Array<int> order(n);
   for (int i=0; i<n; ++i)
      order[i]=i;
   std::sort(order.begin(),order.end(),Comp<Scalar>(weights));
   Set<int> min_base;
   const Array<Set<int> >& circuits=m.give("CIRCUITS");
   std::list<int> checks;
   for (int i=0; i<circuits.size(); ++i)
      checks.push_back(i);
   for (auto eit=entire(order); !eit.at_end(); ++eit) {
      min_base+=(*eit);
      for (std::list<int>::iterator cit=checks.begin(); cit != checks.end(); ++cit) {
         if (circuits[*cit].contains(*eit))
            if (incl(circuits[*cit],min_base) <= 0) {
               min_base-=(*eit);
               for (std::list<int>::iterator it=checks.begin(); it != checks.end(); circuits[*it].contains(*eit) ? it=checks.erase(it) : ++it) {}
               break;
            }
      }
   }
   return min_base;
}

UserFunctionTemplate4perl("# @category Other"
                  "# Calculates a minimal weight basis."
                  "# @param Matroid matroid"
                  "# @param Vector weights for the elements of the matroid"
                  "# @return Set minimal weight basis","minimal_base(Matroid, Vector)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
