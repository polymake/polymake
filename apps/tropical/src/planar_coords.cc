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
#include "polymake/Vector.h"
#include "polymake/Matrix.h"
#include "polymake/linalg.h"
#include <cmath>

namespace polymake { namespace tropical {

Matrix<double> planar_coords(const Matrix<double>& M, perl::OptionSet options)
{
   const int n=M.rows(), d=M.cols();
   Matrix<double> planar_M(n,2);
   
   Matrix<double> directions(d,2);
   if (!(options["Directions"] >> directions))
      for (int k=0; k<d; ++k) {
	 directions(k,0) = sin(k*2*M_PI/d);
	 directions(k,1) = cos(k*2*M_PI/d);
      }

   for (int i=0; i<n; ++i)
      for (int k=0; k<d; ++k) {
	 planar_M(i,0) += M(i,k)*directions(k,0);
	 planar_M(i,1) += M(i,k)*directions(k,1);
      }

   return planar_M;
}

Function4perl(&planar_coords, "planar_coords(Matrix<Float> { Directions => undef })");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
