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

#pragma once

#include "polymake/Set.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/FacetList.h"

namespace polymake { namespace polytope {

/// @retval row indices of (non-facets, hidden equations), first includes second
template <typename IM>
std::pair<Set<Int>, Set<Int>>
compress_incidence(const GenericIncidenceMatrix<IM>& VIF)
{
   Set<Int> non_facets, hidden_equations;
   const Int nv = VIF.cols();
   FacetList facets(nv);

   for (auto f=entire(rows(VIF)); !f.at_end(); ++f) {
      if (f->size() == nv) {
         facets.skip_facet_id();
         non_facets.push_back(f.index());
         hidden_equations.push_back(f.index());
      } else if (! facets.replaceMax(*f, inserter(non_facets))) {
         non_facets.push_back(f.index());
      }
   }
   return std::make_pair(non_facets, hidden_equations);
}

} }


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
