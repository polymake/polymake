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
#include "polymake/Rational.h"
#include "polymake/QuadraticExtension.h"
#include "polymake/Vector.h"
#include "polymake/Matrix.h"
#include "polymake/ListMatrix.h"

namespace polymake { namespace polytope {

template <typename E>
void voronoi(perl::Object p)
{
   const Matrix<E> sites=p.give("SITES");
   const int n=sites.rows(), d=sites.cols();

   // if not all SITES have first coordinate 1, we consider
   // the sites as a vector configuration
   bool is_vector_configuration = false;
   for (int i=0; i<n; ++i) {
      if (sites(i,0) != 1) {
         is_vector_configuration=true;
         break;
      }
   }

   const int poly_dim=d+1+is_vector_configuration;
   Matrix<E> voronoi_ineq(n+1, poly_dim);
   auto ineq_it = concat_rows(voronoi_ineq).begin();

   for (int i=0; i<n; ++i, ++ineq_it) {
      *ineq_it = sqr(sites[i])+(is_vector_configuration-1); // -1 compensates for homogenizing coordinate
      ++ineq_it;
      for (int k=1-is_vector_configuration; k<d; ++k, ++ineq_it)
         *ineq_it = -2 * sites(i, k);

      *ineq_it = 1;
   }

   // since all SITES are required to be distinct, the defining inequalities are, in fact, facets;
   // e.g., for the computation of DELAUNAY_GRAPH it is essential that the first facets correspond
   // to the sites
   *ineq_it = 1;  // facet at infinity, since all remaining elements are still 0

   p.take("FACETS") << voronoi_ineq;
   p.take("AFFINE_HULL") << Matrix<E>(0, poly_dim); // always full-dimensional

   // to simplify subsequent computations we also provide an obvious interior point and other basic information
   Vector<E> rip(poly_dim);
   rip.front()=1;  rip.back()=1;
   p.take("REL_INT_POINT") << rip;
   p.take("FEASIBLE") << true;
   p.take("BOUNDED") << false;

}

FunctionTemplate4perl("voronoi<Scalar>(VoronoiPolyhedron<Scalar>) : void");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
