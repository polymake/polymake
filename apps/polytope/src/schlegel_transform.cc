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

#include "polymake/client.h"
#include "polymake/linalg.h"
#include "polymake/Vector.h"
#include "polymake/Matrix.h"
#include "polymake/Graph.h"
#include "polymake/polytope/schlegel_common.h"

namespace polymake { namespace polytope {

template <typename Scalar>
void schlegel_transform(perl::Object S, perl::Object P)
{
   const Matrix<Scalar> V=P.give("VERTICES");
   const int d=V.cols();
   const Matrix<Scalar> F=P.give("FACETS");
   const Graph<> DG=P.give("DUAL_GRAPH.ADJACENCY");

   const int proj_facet=S.give("FACET");
   const Scalar zoom=S.give("ZOOM");
   const Vector<Scalar> FacetPoint=S.give("FACET_POINT"),
      InnerPoint=S.give("INNER_POINT");

   const Vector<Scalar> ViewRay=FacetPoint-InnerPoint;
   const Scalar alpha=schlegel_nearest_neighbor_crossing(F.minor(DG.adjacent_nodes(proj_facet),All), FacetPoint, ViewRay),
      FxVR=F[proj_facet] * ViewRay;

   Vector<Scalar> ViewPoint = FacetPoint;

   // First transformation step: move FacetPoint to the origin
   Matrix<Scalar> tau = unit_matrix<Scalar>(d);
   tau.row(0).slice(1) = -FacetPoint.slice(1);

   // Second step: move the view point to a far point if it is not already.
   // The projection facet is still fixed
   if (alpha>=0) {
      tau.col(0) -= F[proj_facet] / (zoom*alpha*FxVR);
      ViewPoint += zoom*alpha*ViewRay;
   } else if (zoom!=1) {
      // mapping zoom from (0,1) to (0,+inf)
      ViewPoint += (zoom/(1-zoom))*ViewRay;
      tau.col(0) -= F[proj_facet] / (zoom/(1-zoom)*FxVR);
   }

   // Third step: project the points onto the facet parallel to the view ray (shear)
   Vector<Scalar> v = 0|(F[proj_facet]).slice(1);
   tau = tau * ( unit_matrix<Scalar>(d) -
                 (vector2col(v) * vector2row(ViewRay)) / FxVR );
   
   // Fourth step: move everything back to their original position
   Matrix<Scalar> trans = unit_matrix<Scalar>(d);
   trans.row(0).slice(1) = FacetPoint.slice(1);
   tau = tau*trans;
   

   S.take("TRANSFORM") << tau;
   S.take("VIEWPOINT") << ViewPoint;
}

FunctionTemplate4perl("schlegel_transform<Scalar> (SchlegelDiagram<Scalar>, Polytope<Scalar>) : void");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
