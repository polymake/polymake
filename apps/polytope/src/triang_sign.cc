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

#include "polymake/client.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/Array.h"
#include "polymake/PowerSet.h"
#include "polymake/linalg.h"

namespace polymake { namespace polytope {

template <typename Scalar>
Array<int> triang_sign(const Array< Set<int> >& Triangulation, const GenericMatrix<Scalar>& Points)
{
   Array<int> signs(Triangulation.size());
   Array<int>::iterator s=signs.begin();
   for (typename Entire< Array< Set<int> > >::const_iterator t=entire(Triangulation);  !t.at_end();  ++t, ++s)
      *s=sign(det(Points.minor(*t,All)));
   return signs;
}


// Point C is assumed to be an interior point of a polytope.
// Since we are interested in simplices visible "from the outside", we invert the signs here.

template <typename Scalar>
Array< Array<int> > triang_sign(const Array< Set<int> >& TriangBoundary, const Array<Set<int> >& facet_triangs, const Matrix<Scalar>& Points, const Vector<Scalar>& C)
{
   Array< Array<int> > signs(TriangBoundary.size());
   Array< Array<int> >::iterator sf=signs.begin();
   for (Entire< Array< Set<int> > >::const_iterator facet_t=entire(facet_triangs);  !facet_t.at_end();  ++facet_t, ++sf) {
      sf->resize(facet_t->size());
      Array<int>::iterator s=sf->begin();
      for (typename Entire< Set<int> >::const_iterator t=entire(*facet_t);  !t.at_end();  ++t, ++s)
         *s=-sign(det(Points.minor(TriangBoundary[*t],All) / C));
   }
   return signs;
}

FunctionTemplate4perl("triang_sign(Array, Matrix)");
FunctionTemplate4perl("triang_sign(Array, Array, Matrix, Vector)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
