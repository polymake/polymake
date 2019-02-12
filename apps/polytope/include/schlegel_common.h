/* Copyright (c) 1997-2018
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

#ifndef POLYMAKE_POLYTOPE_SCHLEGEL_COMMON_H
#define POLYMAKE_POLYTOPE_SCHLEGEL_COMMON_H

#include "polymake/GenericMatrix.h"
#include "polymake/GenericVector.h"

namespace polymake { namespace polytope {

/** @retval alpha>=0 : P=FacetPoint+alpha*ViewRay lies on the nearest neighbor facet hyperplane
            alpha<0  : no intersection  OR  FacetPoint violates some of the neighbor facet constraints
*/
template <typename FMatrix, typename FVector, typename VVector, typename E>
E schlegel_nearest_neighbor_crossing (const GenericMatrix<FMatrix,E>& F,
                                      const GenericVector<FVector,E>& FacetPoint,
                                      const GenericVector<VVector,E>& ViewRay
                                      )
{
   E alpha(-1);
   bool constrained=false;
   for (auto nf=entire(rows(F)); !nf.at_end();  ++nf) {
      const E Fr=(*nf)*ViewRay;
      if (Fr<0) {       // intersection on the negative side
         const E alpha_here=-((*nf)*FacetPoint)/Fr;
         if (!constrained || alpha_here < alpha) {
            constrained=true;
            alpha=alpha_here;
         }
      }
   }
   return alpha;
}

// FacetPoint=Origin implicitly
template <typename FMatrix, typename VVector, typename E>
E schlegel_nearest_neighbor_crossing (const GenericMatrix<FMatrix,E>& F,
                                      const GenericVector<VVector,E>& ViewRay
                                      )
{
   E alpha(-1);
   bool constrained=false;
   for (auto nf=entire(rows(F)); !nf.at_end();  ++nf) {
      const E Fr=(*nf)*ViewRay;
      if (Fr<0) {       // intersection on the negative side
         const E alpha_here=-((*nf)[0]/Fr);
         if (!constrained || alpha_here < alpha) {
            constrained=true;
            alpha=alpha_here;
         }
      }
   }
   return alpha;
}

} }

#endif // POLYMAKE_POLYTOPE_SCHLEGEL_COMMON_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
