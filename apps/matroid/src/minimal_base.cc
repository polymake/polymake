/* Copyright (c) 1997-2020
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
Set<Int> minimal_base(BigObject m, const Vector<Scalar>& weights)
{
   Int n = m.give("N_ELEMENTS");
   Array<Int> order(n);
   for (Int i = 0; i < n; ++i)
      order[i] = i;
   std::sort(order.begin(), order.end(), [&](Int k, Int l) { return weights[k] < weights[l]; });
   Set<Int> min_base;
   const Array<Set<Int>>& circuits = m.give("CIRCUITS");
   std::list<Int> checks;
   for (Int i = 0; i < circuits.size(); ++i)
      checks.push_back(i);
   for (auto eit = entire(order); !eit.at_end(); ++eit) {
      min_base += *eit;
      for (auto cit = checks.begin(); cit != checks.end(); ++cit) {
         const Int c = *cit;
         if (circuits[c].contains(*eit)) {
            if (incl(circuits[c], min_base) <= 0) {
               min_base -= *eit;
               for (auto it = checks.begin(); it != checks.end(); circuits[*it].contains(*eit) ? it = checks.erase(it) : ++it) {}
               break;
            }
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
