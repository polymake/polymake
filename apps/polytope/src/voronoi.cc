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
   int vector_configuration = 0;
   for (int i=0; !vector_configuration&&i<n; ++i) {
     if (sites(i,0)!=1) vector_configuration=1;
   }

   ListMatrix< Vector<E> > voronoi_ineq(0,d+1+vector_configuration);

   for (int i=0; i<n; ++i) {
      Vector<E> next_voronoi_ineq=unit_vector<E>(d+1+vector_configuration,d+vector_configuration);
      next_voronoi_ineq[0]=sqr(sites[i])-1+vector_configuration; // -1 compensates for homogenizing coordinate
      for (int k=1-vector_configuration; k<d; ++k) {
         const E& x(sites(i,k));
         next_voronoi_ineq[k]=(-2)*x;
      }
      voronoi_ineq /= next_voronoi_ineq;
   }

   // since all SITES are required to be distinct, the defining inequalities are, in fact, facets;
   // e.g., for the computation of DELAUNAY_GRAPH it is essential that the first facets correspond
   // to the sites
   voronoi_ineq /= unit_vector<E>(d+1,0); // facet at infinity
   p.take("FACETS") << voronoi_ineq;
   p.take("AFFINE_HULL") << Matrix<E>(); // always full-dimensional

   // to simplify subsequent computations we also provide an obvious interior point and other basic information
   Vector<E> rip(d+1); rip[0]=rip[d]=1;
   p.take("REL_INT_POINT") << rip;
   p.take("FEASIBLE") << true;
   p.take("BOUNDED") << false;

}

FunctionTemplate4perl("voronoi<Scalar>(VoronoiDiagram<Scalar>) : void");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
