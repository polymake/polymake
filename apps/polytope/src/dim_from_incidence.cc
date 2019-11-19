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
#include "polymake/IncidenceMatrix.h"
#include "polymake/FacetList.h"

namespace polymake { namespace polytope {
namespace {

template <typename Iterator>
FacetList
ridges_of_first(int n_vertices, Iterator set)
{
   FacetList R(n_vertices);
   Iterator set2=set;

   for (++set2; !set2.at_end(); ++set2)
      R.replaceMax((*set) * (*set2));

   return R;
}
}

int dim_from_incidence(const IncidenceMatrix<>& VIF)
{
   const int n_vertices=VIF.cols();
   if (n_vertices<=3) return n_vertices-1; // return COMBINATORIAL_DIM !

   FacetList F=ridges_of_first(n_vertices, entire(rows(VIF)));
   int d=1;
   // for each n=F.size() <= 3 there is a unique combinatorial class
   // of cones with n facets
   while (F.size() > 3) {
      F=ridges_of_first(n_vertices, entire(F));
      ++d;
   }
   // return COMBINATORIAL_DIM
   return d + F.size() - 1;
}

Function4perl(&dim_from_incidence, "dim_from_incidence");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
